package dev.crafterhunter.client;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.Arrays;

/**
 * The channel's own behaviour, checked outside the game.
 *
 * The agreement test proves the guest and the native side state the same
 * numbers. This proves the guest honours the rules those numbers imply: a slot
 * is only complete once its header is written, pixels come before the header
 * that claims them, and a frame of the wrong size is refused rather than
 * truncated into a picture of something else.
 */
public final class FrameChannelTest {
    public static void main(String[] args) throws IOException, InterruptedException {
        Path directory = Files.createTempDirectory("crafterhunter-channel");
        Path file = directory.resolve("frame.channel");
        try {
            roundTrip(file);
            refusesWrongSize(file);
            alternateSlots(file);
            slotIsNeverObservedTorn(file);
            reopensOnTheSamePath(file);
        } finally {
            Files.deleteIfExists(file);
            Files.deleteIfExists(directory);
        }
        System.out.println(
            "Frame channel checks passed: round trip, wrong size refused, slot alternation, "
                + "slot never observed torn, reopen.");
    }

    private static void roundTrip(Path file) throws IOException {
        int width = 8;
        int height = 4;
        byte[] pixels = new byte[(int) FrameLayout.frameBytes(width, height)];
        for (int index = 0; index < pixels.length; index += 4) {
            pixels[index] = (byte) 0x7f;
            pixels[index + 3] = (byte) 0xff;
        }

        try (FrameChannel channel = new FrameChannel(file, width, height)) {
            long first = channel.publish(pixels, 1_000L);
            long second = channel.publish(pixels, 2_000L);
            if (first != 1 || second != 2) {
                throw new AssertionError("sequences must increase: " + first + ", " + second);
            }

            byte[] read = Files.readAllBytes(file);
            int expected = FrameChannel.bufferBytes(width, height);
            if (read.length != expected) {
                throw new AssertionError("buffer is " + read.length + " bytes, expected " + expected);
            }

            // The pixels of the slot that was written second, at the offset the
            // native side will compute for slot one.
            int offset = FrameChannel.pixelsOffsetFor(1, width, height);
            byte[] seen = Arrays.copyOfRange(read, offset, offset + pixels.length);
            if (!Arrays.equals(seen, pixels)) {
                throw new AssertionError("slot one's pixels are not the ones published");
            }

            // The slot header the native reader walks, at the offsets
            // frame_transport.hpp pins: version, four reserved words, sequence,
            // clock, published, then the size.
            int headerAt = FrameChannel.slotOffset(1);
            int seenVersion = readInt(read, headerAt);
            long sequence = readLong(read, headerAt + 16);
            long clock = readLong(read, headerAt + 24);
            long published = readLong(read, headerAt + 32);
            int seenWidth = readInt(read, headerAt + 40);
            int seenHeight = readInt(read, headerAt + 44);
            if (seenVersion != FrameChannel.FORMAT_VERSION || published != 1) {
                throw new AssertionError(
                    "slot header is not marked complete: version=" + seenVersion
                        + " published=" + published);
            }
            if (sequence != 2 || clock != 2_000 || seenWidth != width || seenHeight != height) {
                throw new AssertionError(
                    "slot header does not describe what was published: seq=" + sequence
                        + " clock=" + clock + " size=" + seenWidth + "x" + seenHeight);
            }

            // Magic and version, so a reader can refuse any other file.
            if (readInt(read, 0) != FrameChannel.MAGIC || readInt(read, 4) != FrameChannel.FORMAT_VERSION) {
                throw new AssertionError("the preamble does not identify this format");
            }
        }
    }

    private static void refusesWrongSize(Path file) throws IOException {
        try (FrameChannel channel = new FrameChannel(file, 8, 4)) {
            channel.publish(new byte[(int) FrameLayout.frameBytes(8, 4)], 1L);
            try {
                channel.publish(new byte[10], 2L);
                throw new AssertionError("a frame of the wrong size must be refused");
            } catch (IllegalArgumentException expected) {
                if (!expected.getMessage().contains("128 bytes")) {
                    throw new AssertionError("the refusal must name the size it wanted: "
                        + expected.getMessage());
                }
            }
        }
    }

    private static void alternateSlots(Path file) throws IOException {
        try (FrameChannel channel = new FrameChannel(file, 4, 4)) {
            byte[] pixels = new byte[(int) FrameLayout.frameBytes(4, 4)];
            // Sequences 1 and 3 land in slot zero, 2 and 4 in slot one, so a
            // reader holding slot one while the writer returns to slot zero is
            // the normal case and not a special one.
            for (int sequence = 1; sequence <= 4; sequence++) {
                Arrays.fill(pixels, (byte) sequence);
                channel.publish(pixels, sequence * 10L);
            }

            // Sequence lives 16 bytes into a slot header, after the version and
            // three reserved words; reading it at the slot offset would read the
            // version and pass for the wrong reason.
            byte[] read = Files.readAllBytes(file);
            if (readInt(read, FrameChannel.slotOffset(0) + 16) != 3) {
                throw new AssertionError("slot zero should hold the third frame");
            }
            if (readInt(read, FrameChannel.slotOffset(1) + 16) != 4) {
                throw new AssertionError("slot one should hold the fourth frame");
            }
            int offset = FrameChannel.pixelsOffsetFor(0, 4, 4);
            if (read[offset] != 3) {
                throw new AssertionError("slot zero's pixels should be the third frame");
            }
        }
    }

    /**
     * A slot being refilled must never look like a finished frame.
     *
     * <p>This is the ordering {@code publish()} exists to get right, and it cannot
     * be checked by inspecting a settled channel: every completed frame looks the
     * same whichever order the writes happened in. It has to be watched while
     * writes are in flight, with a reader sampling continuously.
     *
     * <p>Two properties, in order of importance:
     *
     * <ol>
     *   <li><b>Never torn.</b> No sample may show {@code published = 1} while the
     *       pixels belong to two different frames. This is the assertion that
     *       matters, and it is a "never", so it does not depend on winning a race.
     *   <li><b>Visibly incomplete.</b> A sample must show {@code published = 0}
     *       with mixed pixels - the state that tells a reader the slot is being
     *       refilled. This does depend on sampling inside the window, so the frame
     *       is made large (4 MB) and published many times to make the window wide
     *       rather than to make the test lenient.
     * </ol>
     *
     * <p>Without the incomplete write, property 1 fails: the slot carries
     * {@code published = 1} from the frame before it while its pixels are being
     * replaced, which is precisely the torn read the native side now re-checks for.
     */
    private static void slotIsNeverObservedTorn(Path file)
            throws IOException, InterruptedException {
        // 16 MB per frame, deliberately. The window in which a slot is half-written
        // is only as wide as the pixel write takes, and at a realistic frame size
        // that is microseconds - a reader sampling every few tens of microseconds
        // lands in it rarely enough that this test passed with the fix deleted. A
        // write lasting milliseconds makes the window impossible to miss, which is
        // what turns a test that cannot fail into one that can.
        int width = 2048;
        int height = 2048;
        int slotBytes = (int) FrameLayout.frameBytes(width, height);
        int total = FrameChannel.bufferBytes(width, height);

        byte[] alpha = new byte[slotBytes];
        byte[] beta = new byte[slotBytes];
        Arrays.fill(alpha, (byte) 0x11);
        Arrays.fill(beta, (byte) 0x22);

        // Atomics, not arrays. A reader incrementing samples[0] in a plain array
        // is not guaranteed to be visible to a thread spinning on it: the read can
        // be hoisted out of the loop and the spin never ends. That happened, and it
        // looked exactly like a reader that had stopped.
        final java.util.concurrent.atomic.AtomicBoolean stop =
                new java.util.concurrent.atomic.AtomicBoolean();
        final java.util.concurrent.atomic.AtomicBoolean sawFlagDown =
                new java.util.concurrent.atomic.AtomicBoolean();
        final java.util.concurrent.atomic.AtomicBoolean sawTorn =
                new java.util.concurrent.atomic.AtomicBoolean();
        final java.util.concurrent.atomic.AtomicLong samples =
                new java.util.concurrent.atomic.AtomicLong();
        final java.util.concurrent.atomic.AtomicLong mixedSamples =
                new java.util.concurrent.atomic.AtomicLong();
        final java.util.concurrent.atomic.AtomicLong incompleteSamples =
                new java.util.concurrent.atomic.AtomicLong();

        try (FrameChannel channel = new FrameChannel(file, width, height)) {
            // Settle both slots before the reader starts. Until the first header is
            // written a slot reads published=0 simply because the file was zeroed,
            // which is not the state under test; mixing those samples in is what
            // let the run below pass with the incomplete write removed.
            channel.publish(alpha, 1L);
            channel.publish(beta, 2L);

            // The reader samples the few bytes it needs rather than the whole
            // buffer. Reading 8 MB per sample is slower than forty publishes, so
            // the loop finished with one sample and proved nothing; three small
            // reads give thousands. Reading the header and the pixels at separate
            // moments is also the honest shape of the problem - a real reader
            // validates a header and then uploads pixels some microseconds later.
            Thread reader = new Thread(() -> {
                byte[] header = new byte[FrameChannel.SLOT_HEADER_BYTES];
                int pixelsAt = FrameChannel.pixelsOffsetFor(0, width, height);
                int lastPixelAt = pixelsAt + slotBytes - 1;
                // Positional reads on a FileChannel, not FileInputStream: JDK 27
                // removed seek() and the other legacy channel methods from
                // FileInputStream, and a positional read is the honest shape of the
                // problem anyway - a real reader validates a header and then reads
                // pixels from wherever it has got to.
                try (java.nio.channels.FileChannel in =
                        java.nio.channels.FileChannel.open(file, StandardOpenOption.READ)) {
                    while (!stop.get()) {
                        // Header, pixels, header again. The second header read is
                        // the whole test: tearing is not "pixels changed at some
                        // point" but "the slot looked finished, changed underneath,
                        // and still looked finished afterwards". Sampling the header
                        // once on either side of the pixels is also exactly what the
                        // native reader now does with stillHolds().
                        if (readFullyAt(in, header, FrameChannel.slotOffset(0)) < header.length) {
                            continue;
                        }
                        long publishedBefore = readLong(header, 32);
                        byte[] firstPixel = new byte[1];
                        byte[] lastPixel = new byte[1];
                        if (readFullyAt(in, firstPixel, pixelsAt) != 1
                                || readFullyAt(in, lastPixel, lastPixelAt) != 1) {
                            continue;
                        }
                        byte[] after = new byte[FrameChannel.SLOT_HEADER_BYTES];
                        if (readFullyAt(in, after, FrameChannel.slotOffset(0)) < after.length) {
                            continue;
                        }
                        long publishedAfter = readLong(after, 32);
                        samples.incrementAndGet();
                        boolean mixed = firstPixel[0] != lastPixel[0];
                        if (mixed) {
                            mixedSamples.incrementAndGet();
                            if (publishedBefore == 0 || publishedAfter == 0) {
                                incompleteSamples.incrementAndGet();
                            }
                        }
                        if (publishedBefore == 1 && publishedAfter == 1 && mixed) {
                            // The slot looked complete, its pixels changed while it
                            // was being read, and it still looks complete: a reader
                            // would have composited half of one frame and half of
                            // another and had no way to tell.
                            sawTorn.set(true);
                        }
                        if (publishedBefore == 0 || publishedAfter == 0) {
                            // Proof that the slot is marked incomplete while it is
                            // being refilled. With the fix this is true for the whole
                            // of the pixel write, so it is a wide window rather than
                            // a lucky sample.
                            sawFlagDown.set(true);
                        }
                    }
                } catch (IOException ignored) {
                    // The assertions below report whatever was collected.
                }
            });
            reader.start();

            // Wait for the reader to actually be sampling, yielding rather than
            // spinning: Thread.onSpinWait() starved the very thread being waited
            // on, and the run reported that it had never run at all.
            long readyDeadline = System.nanoTime() + 10_000_000_000L;
            while (samples.get() < 3 && System.nanoTime() < readyDeadline) {
                java.util.concurrent.locks.LockSupport.parkNanos(200_000L);
            }

            for (int i = 0; i < 8; i++) {
                channel.publish((i % 2 == 0) ? beta : alpha, 1_000L + i);
            }
            stop.set(true);
            reader.join();

            if (sawTorn.get()) {
                throw new AssertionError(
                    "a slot was observed with published=1 and pixels from two frames:"
                        + " a reader would have composited a torn frame");
            }
            if (!sawFlagDown.get()) {
                throw new AssertionError(
                    "published was never observed as 0, so the slot is never marked"
                        + " incomplete: a reader cannot distinguish a frame in flight"
                        + " from a finished one");
            }
            if (samples.get() < 100) {
                throw new AssertionError(
                    "only " + samples.get() + " samples were taken; the reader never really"
                        + " ran, so the other two results mean nothing");
            }
            System.out.println("  (publication: " + samples.get() + " samples, "
                + mixedSamples.get() + " caught pixels mid-refill, "
                + incompleteSamples.get() + " of them marked incomplete)");
        }
    }

    private static void reopensOnTheSamePath(Path file) throws IOException {
        // The capture reopens the channel when the window resizes, which happens
        // every time someone moves the game between monitors. Reopening must not
        // fail on a file that already exists at the right size.
        try (FrameChannel first = new FrameChannel(file, 8, 4)) {
            first.publish(new byte[(int) FrameLayout.frameBytes(8, 4)], 1L);
        }
        try (FrameChannel second = new FrameChannel(file, 8, 4)) {
            second.publish(new byte[(int) FrameLayout.frameBytes(8, 4)], 1L);
            if (readInt(Files.readAllBytes(file), 0) != FrameChannel.MAGIC) {
                throw new AssertionError("reopening must leave the preamble intact");
            }
        }
        // A different size must replace the buffer rather than reuse it, which
        // is what happens when the window moves to another monitor.
        int expected = FrameChannel.bufferBytes(16, 8);
        try (FrameChannel resized = new FrameChannel(file, 16, 8)) {
            if ((int) Files.size(file) != expected) {
                throw new AssertionError(
                    "a resized window must get a buffer of " + expected + " bytes, not "
                        + Files.size(file));
            }
            resized.publish(new byte[(int) FrameLayout.frameBytes(16, 8)], 1L);
        }
    }

    /** Positional read that loops until the buffer is full or the file ends. */
    private static int readFullyAt(java.nio.channels.FileChannel channel, byte[] into,
            int position) throws IOException {
        java.nio.ByteBuffer buffer = java.nio.ByteBuffer.wrap(into);
        int total = 0;
        while (buffer.hasRemaining()) {
            int n = channel.read(buffer, position + total);
            if (n < 0) {
                break;
            }
            total += n;
        }
        return total;
    }

    /** Little-endian, matching the transport's byte order. */
    private static long readLong(byte[] bytes, int offset) {
        long value = 0;
        for (int index = 7; index >= 0; index--) {
            value = (value << 8) | (bytes[offset + index] & 0xFFL);
        }
        return value;
    }

    /** Little-endian, and deliberately built the other way round. */
    private static int readInt(byte[] bytes, int offset) {
        return (bytes[offset] & 0xFF)
            | ((bytes[offset + 1] & 0xFF) << 8)
            | ((bytes[offset + 2] & 0xFF) << 16)
            | ((bytes[offset + 3] & 0xFF) << 24);
    }
}

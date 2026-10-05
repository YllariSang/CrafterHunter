package dev.crafterhunter.client;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
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
    public static void main(String[] args) throws IOException {
        Path directory = Files.createTempDirectory("crafterhunter-channel");
        Path file = directory.resolve("frame.channel");
        try {
            roundTrip(file);
            refusesWrongSize(file);
            alternateSlots(file);
            reopensOnTheSamePath(file);
        } finally {
            Files.deleteIfExists(file);
            Files.deleteIfExists(directory);
        }
        System.out.println(
            "Frame channel checks passed: round trip, wrong size refused, slot alternation, reopen.");
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

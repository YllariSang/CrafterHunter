package dev.crafterhunter.client;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.channels.FileChannel;

/**
 * The guest half of the shared-memory frame contract.
 *
 * The rules here are stated twice on purpose: {@code frame_transport.hpp} on the
 * native side and this class on the guest side. {@code
 * tools/test-frame-transport-agreement.sh} compiles both and compares the
 * numbers, so a layout change on one side fails a test rather than producing a
 * composited frame that is subtly wrong — flipped, offset, or a pixel behind.
 *
 * Two slots, one frame in flight. The transport's job is to hand MHW's renderer
 * the newest complete frame; a second copy in flight would cost bandwidth to
 * reorder anyway. A slot is published by writing its pixels, then a barrier,
 * then its header — so a reader that sees a published header is guaranteed to
 * see whole pixels, never a half-written frame.
 */
public final class FrameChannel implements AutoCloseable {
    /** Matches FormatVersion in frame_transport.hpp. */
    public static final int FORMAT_VERSION = 1;

    /** Matches FormatName in frame_transport.hpp. */
    public static final String FORMAT_NAME = "rgba8-topdown";

    public static final int BYTES_PER_PIXEL = 4;
    public static final int SLOT_COUNT = 2;

    /** Matches Magic in frame_transport.hpp. */
    public static final int MAGIC = 0x43484652;

    /** Matches {@code sizeof(BufferHeader)}: magic, version, and reserved room. */
    public static final int HEADER_BYTES = 128;

    /**
     * Matches {@code sizeof(SlotHeader)} on the native side, not the size of its
     * fields.
     *
     * Seven longs and four ints would be 40 bytes, and the struct's
     * {@code alignas(64)} pads it to 64. A guest that laid out the header from
     * its field count would put the second slot's header inside the first slot's
     * pixels. {@code tools/test-frame-transport-agreement.sh} exists to catch
     * exactly that, and caught it the first time it ran.
     */
    public static final int SLOT_HEADER_BYTES = 64;

    /**
     * Matches {@code MaxAgeNanos} on the native side: past this a frame is
     * stale, and the guest says so rather than leaving the reader to notice a
     * world that has moved on.
     */
    public static final long MAX_AGE_NANOS = 50_000_000L;

    private static final int HEADER_OFFSETS = 15;

    private final Path path;
    private final FileChannel channel;
    private final int width;
    private final int height;
    private final int slotBytes;
    private final int pixelsOffset;
    private long nextSequence = 1;

    public FrameChannel(Path path, int width, int height) throws java.io.IOException {
        this.path = path;
        this.width = width;
        this.height = height;
        this.slotBytes = (int) FrameLayout.frameBytes(width, height);
        this.pixelsOffset = HEADER_BYTES + SLOT_COUNT * SLOT_HEADER_BYTES;

        Files.createDirectories(path.getParent());
        int total = bufferBytes(width, height);
        if (!Files.exists(path) || (int) Files.size(path) != total) {
            Files.deleteIfExists(path);
            Files.write(path, new byte[total], StandardOpenOption.CREATE_NEW);
        }
        this.channel = FileChannel.open(path, StandardOpenOption.READ, StandardOpenOption.WRITE);

        // The preamble is written once: magic and version, so a reader can tell
        // this apart from any other file at the same path.
        // ByteBuffer.allocate is big-endian, and every field in this file is
        // little-endian because that is what the native reader expects. Without
        // this line the writer produces a buffer that is byte-swapped rather
        // than wrong: every field is present, at the right offset, holding the
        // right value, and useless.
        java.nio.ByteBuffer preamble = java.nio.ByteBuffer.allocate(HEADER_BYTES)
            .order(java.nio.ByteOrder.LITTLE_ENDIAN);
        preamble.putInt(MAGIC).putInt(FORMAT_VERSION);
        while (preamble.position() < HEADER_BYTES) {
            preamble.put((byte) 0);
        }
        preamble.flip();
        channel.write(preamble, 0);
    }

    /** Bytes a buffer of this size needs; must match bufferBytes in the header. */
    public static int bufferBytes(int width, int height) {
        return HEADER_BYTES + SLOT_COUNT * SLOT_HEADER_BYTES
            + SLOT_COUNT * (int) FrameLayout.frameBytes(width, height);
    }

    /** Byte offset of a slot header; must match slotOffset in the header. */
    public static int slotOffset(int index) {
        return HEADER_BYTES + index * SLOT_HEADER_BYTES;
    }

    /** Byte offset of a slot's pixels; must match pixelsOffset in the header. */
    public static int pixelsOffsetFor(int index, int width, int height) {
        return HEADER_BYTES + SLOT_COUNT * SLOT_HEADER_BYTES
            + index * (int) FrameLayout.frameBytes(width, height);
    }

    /**
     * Publish a frame into the slot that is not being read.
     *
     * Three writes, and the order is the whole contract:
     *
     * <ol>
     *   <li>mark the slot incomplete, keeping the sequence it already had;
     *   <li>write the pixels;
     *   <li>write the header with {@code published = 1}.
     * </ol>
     *
     * The first write is the one that was missing. Without it a slot being
     * refilled still carries {@code published = 1} from the frame before it, so a
     * reader that decided to upload from this slot - perfectly legal, since the
     * header looks complete - would be reading pixels that are being overwritten
     * underneath it, and would composite a frame half old and half new. With it,
     * a reader can tell the difference between a finished frame and one in
     * flight, and skip the second.
     *
     * Nothing above step 3 may be reordered below it: the pixels have to be
     * visible before the header that claims they are complete. Every write here
     * goes through {@code FileChannel.write}, so the ordering is the kernel's to
     * preserve rather than the compiler's, and a reader mapping the same file
     * sees the same pages. No {@code force} is issued: both processes are on this
     * host and share one page cache, so a flush would cost a 7.8 MB round trip
     * per frame to achieve nothing.
     *
     * @return the sequence this frame was published under
     */
    public long publish(byte[] rgbaTopDown, long capturedNanos) throws java.io.IOException {
        if (rgbaTopDown.length != slotBytes) {
            throw new IllegalArgumentException(
                "A " + width + "x" + height + " frame is " + slotBytes
                    + " bytes, got " + rgbaTopDown.length);
        }

        int slot = (int) ((nextSequence - 1) % SLOT_COUNT);
        int pixelsAt = pixelsOffsetFor(slot, width, height);

        // Step 1: incomplete, sequence unchanged so a reader can still tell which
        // frame this slot used to hold.
        writeFully(slotHeader(nextSequence, capturedNanos, 0), slotOffset(slot));

        // Step 2: the pixels, which may take several writes.
        java.nio.ByteBuffer pixels = java.nio.ByteBuffer.wrap(rgbaTopDown);
        while (pixels.hasRemaining()) {
            channel.write(pixels, pixelsAt + pixels.position());
        }

        // Step 3: complete.
        writeFully(slotHeader(nextSequence, capturedNanos, 1), slotOffset(slot));

        long published = nextSequence;
        nextSequence++;
        return published;
    }

    /**
     * One slot header, padded out to the cache line the native struct is aligned
     * to. The padding is zeroed so a reader never sees bytes left over from an
     * earlier frame.
     */
    private java.nio.ByteBuffer slotHeader(long sequence, long capturedNanos,
            long publishedFlag) {
        java.nio.ByteBuffer header = java.nio.ByteBuffer.allocate(SLOT_HEADER_BYTES)
            .order(java.nio.ByteOrder.LITTLE_ENDIAN);
        header.putInt(FORMAT_VERSION).putInt(0).putInt(0).putInt(0);
        header.putLong(sequence).putLong(capturedNanos).putLong(publishedFlag);
        header.putInt(width).putInt(height);
        while (header.position() < SLOT_HEADER_BYTES) {
            header.put((byte) 0);
        }
        header.flip();
        return header;
    }

    /**
     * Writes the whole buffer, looping until it is out.
     *
     * {@code FileChannel.write} is permitted to write fewer bytes than it was
     * given and to write none at all. The pixel loop always checked; the header
     * write did not, and a partial header would leave the reader looking at a
     * mixture of this frame's fields and the previous one's - which, for a 64-byte
     * header, means a plausible sequence number beside a stale {@code published}
     * flag.
     */
    private void writeFully(java.nio.ByteBuffer buffer, long position) throws java.io.IOException {
        while (buffer.hasRemaining()) {
            channel.write(buffer, position + buffer.position());
        }
    }

    /** Remove the channel's file, so a reader stops trusting it. */
    @Override
    public void close() throws java.io.IOException {
        channel.close();
        Files.deleteIfExists(path);
    }
}

package dev.crafterhunter.client;

/**
 * The shared-memory frame contract, kept pure so it can be checked outside the
 * game.
 *
 * A published frame is {@code width * height * 4} bytes of RGBA8 in top-down row
 * order, beside a small text file naming the sequence it belongs to. Row order
 * is fixed here rather than left to the reader because a GPU copy hands back
 * rows bottom-up on an OpenGL backend and top-down on Vulkan; a consumer that
 * guessed would show MHW's sky at the bottom of Steve's world on one driver and
 * the right way up on the other.
 */
public final class FrameLayout {
    /** RGBA8, one row-major frame, top-down. */
    public static final String FORMAT = "rgba8-topdown";

    private FrameLayout() {
    }

    /**
     * Copy {@code source} into a new array with the rows in the other order.
     *
     * A frame read straight off a GPU arrives with row zero at the bottom; this
     * is what makes it a normal image.
     */
    public static byte[] flipRows(byte[] source, int width, int height) {
        if (source == null) {
            throw new IllegalArgumentException("A frame needs pixels");
        }
        int stride = width * 4;
        if (width <= 0 || height <= 0 || source.length < stride * height) {
            throw new IllegalArgumentException(
                "A " + width + "x" + height + " frame needs " + (stride * height)
                    + " bytes, got " + source.length);
        }
        byte[] flipped = new byte[stride * height];
        for (int row = 0; row < height; row++) {
            System.arraycopy(
                source,
                (height - 1 - row) * stride,
                flipped,
                row * stride,
                stride);
        }
        return flipped;
    }

    /** The meta line describing a published frame. */
    public static String meta(long sequence, long capturedAtNanos, int width, int height) {
        if (sequence < 0 || capturedAtNanos < 0 || width <= 0 || height <= 0) {
            throw new IllegalArgumentException("A frame needs a sequence, a clock and a size");
        }
        return "sequence=" + sequence
            + " capturedNanos=" + capturedAtNanos
            + " width=" + width
            + " height=" + height
            + " format=" + FORMAT
            + System.lineSeparator();
    }

    /** One field of a parsed meta line, or -1 when the line does not carry it. */
    public static long field(String meta, String name) {
        if (meta == null || name == null) {
            return -1;
        }
        for (String part : meta.split("\\s+")) {
            if (part.startsWith(name + "=")) {
                try {
                    return Long.parseLong(part.substring(name.length() + 1));
                } catch (NumberFormatException exception) {
                    return -1;
                }
            }
        }
        return -1;
    }

    /** The bytes a frame of this size must contain. */
    public static long frameBytes(int width, int height) {
        if (width <= 0 || height <= 0) {
            throw new IllegalArgumentException("A frame needs a positive size");
        }
        return (long) width * height * 4L;
    }
}

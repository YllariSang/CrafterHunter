package dev.crafterhunter.client;

/**
 * The row and format contract of the depth readback, stated without a single GL
 * type in it so that it can be checked on any machine.
 *
 * <p>It exists because two of these numbers were previously *inferred* from the
 * wrong quantity. The reader reported {@code depthBufferBytes}, which is the
 * capacity it allocated, and derived {@code depthRowsPadded} from comparing that
 * capacity with {@code width*height*4} - a comparison that can only ever come
 * out false, since capacity is exactly that product by construction. Buffer
 * capacity is not row stride: a stride is decided by the pixel-store state the
 * reader sets, and it is that state, and not the size of the destination, which
 * decides whether row <i>n</i> starts {@code n*stride} bytes in.
 *
 * <p>What the driver actually does with that state is measured by
 * {@code tools/test-depth-readback.sh}, which reads a known pattern with sentinels
 * around it and fails if a row lands anywhere but tightly. This class holds the
 * arithmetic that measurement is checking, and the names the metadata is written
 * with.
 */
public final class DepthReadbackPlan {

    /** D32_FLOAT, R32_FLOAT and every depth format read as four bytes a texel. */
    public static final int BYTES_PER_TEXEL = 4;

    /**
     * The alignment this reader sets before reading.
     *
     * <p>Set rather than assumed, because a driver's default is a fact about the
     * driver and a row whose stride is not a multiple of it gains padding bytes.
     */
    public static final int PACK_ALIGNMENT = 4;

    private DepthReadbackPlan() {
    }

    /**
     * Bytes between the first texel of one row and the first texel of the next.
     *
     * <p>The reader sets {@code GL_PACK_ROW_LENGTH = width}, so this is
     * {@code width * 4} and nothing else can influence it - in particular not how
     * much room the destination happens to have.
     */
    public static long rowStrideBytes(int width) {
        return (long) width * BYTES_PER_TEXEL;
    }

    /**
     * Bytes one row occupies once alignment padding is added.
     *
     * <p>{@code rowStrideBytes} is the distance between rows; padding is extra
     * that a driver may append <i>within</i> a row when the stride is not a
     * multiple of the alignment. They differ only when the alignment does not
     * divide the stride.
     */
    public static long rowBytesWithPadding(int width, int packAlignment) {
        long stride = rowStrideBytes(width);
        if (packAlignment <= 1) {
            return stride;
        }
        long remainder = stride % packAlignment;
        return remainder == 0 ? stride : stride + (packAlignment - remainder);
    }

    /** Bytes a whole-frame read writes. This is the number to size against, not capacity. */
    public static long writtenBytes(int width, int height, int packAlignment) {
        return rowBytesWithPadding(width, packAlignment) * (long) height;
    }

    /**
     * Whether a read with this pixel-store state lands rows further apart than
     * {@link #rowStrideBytes(int)}.
     *
     * <p>Both ways a stride goes wrong are covered, and each produces its own
     * silent damage: a row length larger than the width shears every row after
     * the first, and alignment padding does the same thing at a smaller scale.
     */
    public static boolean rowsPadded(int packRowLength, int width, int packAlignment) {
        return packRowLength != width || rowBytesWithPadding(packRowLength, packAlignment)
            != rowStrideBytes(packRowLength);
    }

    /**
     * Where row 0 of a GL readback sits in the frame.
     *
     * <p>Measured, not reasoned: {@code tools/test-depth-readback.sh} writes a
     * distinct value into the top and bottom halves of a framebuffer and reports
     * which half comes back first. GL's read origin is the framebuffer's bottom
     * left, so row 0 is the <b>bottom</b> row of the image - the opposite of what
     * the metadata used to claim, and a claim that would mirror every distance
     * computed from it if it were taken at face value.
     */
    public static String rowOrder() {
        return "bottom-up";
    }

    /**
     * The window-depth convention these values use.
     *
     * <p>Standard GL window depth: 0 at the near plane, 1 at the far plane. MHW's
     * scene depth is reversed-Z, and the two are not compared until each has been
     * normalised in its own space - see {@code tools/validate-world-depth.py},
     * which refuses to do the comparison at all.
     */
    public static String depthConvention() {
        return "gl-window-depth";
    }

    /** {@code glGetFramebufferStatus} value to a name the metadata can carry. */
    public static String framebufferStatusName(int status) {
        return switch (status) {
            case 0x8CD5 -> "GL_FRAMEBUFFER_COMPLETE";
            case 0x8CD6 -> "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            case 0x8CD7 -> "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            case 0x8CDB -> "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            case 0x8CDC -> "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            case 0x8CDD -> "GL_FRAMEBUFFER_UNSUPPORTED";
            case 0x8219 -> "GL_FRAMEBUFFER_UNDEFINED";
            case 0x8DA6 -> "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            case 0x8DA7 -> "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
            case 0 -> "not queried";
            default -> String.format(java.util.Locale.ROOT, "unknown(0x%04X)", status);
        };
    }

    /** {@code glGetError} value to a name. 1286 is the one the live run produced. */
    public static String glErrorName(int error) {
        return switch (error) {
            case 0 -> "GL_NO_ERROR";
            case 0x0500 -> "GL_INVALID_ENUM";
            case 0x0501 -> "GL_INVALID_VALUE";
            case 0x0502 -> "GL_INVALID_OPERATION";
            case 0x0505 -> "GL_OUT_OF_MEMORY";
            case 0x0506 -> "GL_INVALID_FRAMEBUFFER_OPERATION";
            case 0x0507 -> "GL_STACK_OVERFLOW";
            case 0x0508 -> "GL_STACK_UNDERFLOW";
            default -> String.format(java.util.Locale.ROOT, "unknown(0x%04X)", error);
        };
    }
}

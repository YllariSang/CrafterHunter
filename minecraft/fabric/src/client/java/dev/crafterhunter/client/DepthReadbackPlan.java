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

    /**
     * The pixel-pack state this reader sets before reading, and the GL defaults it
     * must restore after.
     *
     * <p>Every value below is set, not inherited. Row length and alignment were
     * always set; the rest were discovered to be just as load-bearing: a leftover
     * {@code GL_PACK_SKIP_ROWS/SKIP_PIXELS} moves the whole read deeper into the
     * destination - past the end of a buffer sized exactly to the frame - and a
     * leftover {@code GL_PACK_SWAP_BYTES} byte-swaps every float on the way in.
     * Neither changes a single GL error code; both change every number. The
     * incoming state is therefore read first, reported in the metadata, set to
     * these values, and put back afterwards - see {@link #nonDefaultPack}.
     *
     * <p>Measured, not reasoned: {@code tools/test-depth-readback.sh} section D
     * starts from a deliberately hostile pack state, shows this reader's previous
     * two-parameter contract corrupted by it, and then shows the guarded read come
     * back correct, in bounds, with every parameter restored.
     */
    public static final int PACK_SKIP_ROWS = 0;

    /** See {@link #PACK_SKIP_ROWS}; a leftover skip shifts the read by texels. */
    public static final int PACK_SKIP_PIXELS = 0;

    /** See {@link #PACK_SKIP_ROWS}; byte-swapped floats are wrong, not errored. */
    public static final boolean PACK_SWAP_BYTES = false;

    /**
     * Bit order within a byte. Ignored by a {@code GL_FLOAT} read, but set anyway
     * so the read runs in a fully known state rather than in whatever the last
     * writer of this context happened to leave.
     */
    public static final boolean PACK_LSB_FIRST = false;

    private DepthReadbackPlan() {
    }

    /**
     * Reports the incoming pixel-pack state wherever it differs from GL's
     * defaults.
     *
     * <p>An empty string means the state was already at its defaults - which is
     * the only state the read's own arithmetic assumes, so nothing was at risk.
     * Anything else is what the read would have inherited had it set only row
     * length and alignment, and it is quoted in the capture metadata: a refusal to
     * publish says the read failed, while this says what it was protected against.
     *
     * <p>The comparison is against the <b>defaults</b>, not against the values
     * this reader sets: the previous row length is 0 (default) rather than the
     * width in normal operation, and flagging that every frame would bury the one
     * frame where something else was actually wrong.
     */
    public static String nonDefaultPack(int rowLength, int alignment, int skipRows, int skipPixels,
            boolean swapBytes, boolean lsbFirst) {
        StringBuilder reported = new StringBuilder();
        if (rowLength != 0) {
            append(reported, "rowLength", rowLength);
        }
        if (alignment != 4) {
            append(reported, "alignment", alignment);
        }
        if (skipRows != PACK_SKIP_ROWS) {
            append(reported, "skipRows", skipRows);
        }
        if (skipPixels != PACK_SKIP_PIXELS) {
            append(reported, "skipPixels", skipPixels);
        }
        if (swapBytes != PACK_SWAP_BYTES) {
            append(reported, "swapBytes", swapBytes);
        }
        if (lsbFirst != PACK_LSB_FIRST) {
            append(reported, "lsbFirst", lsbFirst);
        }
        return reported.length() == 0 ? "" : reported.toString();
    }

    private static void append(StringBuilder reported, String name, Object value) {
        if (reported.length() > 0) {
            reported.append(',');
        }
        reported.append(name).append('=').append(value);
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

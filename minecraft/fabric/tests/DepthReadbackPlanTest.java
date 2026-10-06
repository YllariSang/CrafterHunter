package dev.crafterhunter.client;

/**
 * The row contract the depth reader publishes, checked without a driver.
 *
 * <p>Two of these assertions exist because the metadata used to be derived from the
 * wrong quantity. {@code depthRowsPadded} was computed as
 * {@code capacity > width*height*4}, which is false for every buffer this class
 * allocates - capacity <i>is</i> {@code width*height*4} - so the field could never
 * have reported a padded row, and a driver writing past the frame would have gone
 * unnoticed. The assertions below take the capacity away from the arithmetic
 * entirely and pin the one case the old form got wrong.
 */
public final class DepthReadbackPlanTest {

    public static void main(String[] args) {
        // One row, and a whole frame, at the size the capture actually runs at.
        check(DepthReadbackPlan.rowStrideBytes(1908) == 7632L,
            "a row is width texels of four bytes");
        check(DepthReadbackPlan.writtenBytes(1908, 1028, 4) == 7632L * 1028,
            "a frame is one row times the height");

        // Capacity is not an input to either calculation. This is the assertion that
        // fails if the stride is ever taken from the destination's size instead of
        // from the pixel-store state: doubling the capacity changes nothing about
        // where row 17 starts.
        long tight = DepthReadbackPlan.writtenBytes(1908, 1028, 4);
        check(tight * 2 > tight, "the premise: a buffer can be larger than the frame");
        check(DepthReadbackPlan.rowStrideBytes(1908) * 1028 == tight,
            "capacity is not row stride: a twice-as-large buffer still writes width*4 per row");

        // Padding is reported when it can happen, which the capacity comparison could
        // never do.
        check(!DepthReadbackPlan.rowsPadded(1908, 1908, 4),
            "width*4 is a multiple of four, so nothing is padded");
        check(DepthReadbackPlan.rowsPadded(1912, 1908, 4),
            "a row length longer than the width shears every row after the first");
        check(DepthReadbackPlan.rowsPadded(1909, 1909, 8),
            "an alignment that does not divide width*4 adds bytes inside the row");
        check(DepthReadbackPlan.rowBytesWithPadding(1909, 8) == 7640L,
            "7636 bytes of texels round up to the next multiple of eight");

        // The names the metadata carries are part of the contract: the validators read
        // them, and a wrong name is a confident wrong claim in a file.
        check(DepthReadbackPlan.rowOrder().equals("bottom-up"),
            "row 0 of a GL readback is the framebuffer's bottom row");
        check(DepthReadbackPlan.depthConvention().equals("gl-window-depth"),
            "0 at the near plane, 1 at the far plane - never MHW's reversed Z");
        check(DepthReadbackPlan.framebufferStatusName(0x8CD5).equals("GL_FRAMEBUFFER_COMPLETE"),
            "a complete framebuffer must say so");
        check(DepthReadbackPlan.framebufferStatusName(0x8CD6)
                .equals("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"),
            "the status the game's own depth recipe produces");
        check(DepthReadbackPlan.framebufferStatusName(0).equals("not queried"),
            "an unqueried status must not read as a name");
        // 0x0506 = 1286, the error the live run logged.
        check(DepthReadbackPlan.glErrorName(1286).equals("GL_INVALID_FRAMEBUFFER_OPERATION"),
            "the error the refused depth read produced");

        // The full pack state the reader sets. Each value is pinned because the
        // read's correctness depends on all of them being SET, not inherited: a
        // leftover skip moves the read past the end of a frame-sized buffer, and a
        // leftover byte-swap reverses every float - both without a GL error.
        check(DepthReadbackPlan.PACK_SKIP_ROWS == 0, "no rows are skipped on the way in");
        check(DepthReadbackPlan.PACK_SKIP_PIXELS == 0, "no pixels are skipped on the way in");
        check(!DepthReadbackPlan.PACK_SWAP_BYTES, "floats are not byte-swapped on the way in");
        check(!DepthReadbackPlan.PACK_LSB_FIRST, "bit order within a byte is the default");
        check(DepthReadbackPlan.PACK_ALIGNMENT == 4, "alignment is set, not inherited");

        // The report of an incoming state. All-default reports nothing: the read
        // assumes the defaults, so defaults are the one case with nothing at risk.
        check(DepthReadbackPlan.nonDefaultPack(0, 4, 0, 0, false, false).isEmpty(),
            "the GL default pack state must report as nothing to defend against");
        // The previous contract (row length + alignment only) would have inherited
        // all three of these. Each is named, in order, with its value.
        check(DepthReadbackPlan.nonDefaultPack(0, 4, 5, 7, false, false)
                .equals("skipRows=5,skipPixels=7"),
            "a leftover skip must be named, with its exact value");
        check(DepthReadbackPlan.nonDefaultPack(0, 4, 0, 0, true, false)
                .equals("swapBytes=true"),
            "a leftover byte-swap must be named");
        check(DepthReadbackPlan.nonDefaultPack(17, 1, 0, 0, false, true)
                .equals("rowLength=17,alignment=1,lsbFirst=true"),
            "row length, alignment and bit order are reported in that order");
        // A non-zero previous ROW LENGTH is the default for a reader that never set
        // one, so it counts as non-default here - and so does the width the reader
        // itself set, which is why the comparison is stated against defaults.
        check(DepthReadbackPlan.nonDefaultPack(1908, 4, 0, 0, false, false)
                .equals("rowLength=1908"),
            "anything other than the GL default row length is reported");

        System.out.println("Depth readback plan checks passed: stride from pack state, capacity "
            + "excluded, padding detected, row order and status names pinned, pack state "
            + "guarded and reported.");
    }

    private static void check(boolean value, String what) {
        if (!value) {
            throw new AssertionError(what);
        }
    }
}

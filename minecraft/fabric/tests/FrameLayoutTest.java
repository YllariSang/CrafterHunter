import java.nio.file.Files;
import java.nio.file.Path;
import dev.crafterhunter.client.FrameLayout;
import dev.crafterhunter.client.FrameRequest;

/**
 * Headless checks for the frame contract. These run outside the game on purpose:
 * row order and the meta line are the two things the native side will trust, and
 * a flipped frame or a misparsed sequence is invisible until it is composited
 * into MHW and drawn upside down.
 */
public final class FrameLayoutTest {
    public static void main(String[] args) throws Exception {
        // A 2x3 frame with each row filled with its own row index, so a flip is
        // observable rather than merely symmetric.
        int width = 2;
        int height = 3;
        byte[] source = new byte[width * height * 4];
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                int offset = (row * width + column) * 4;
                source[offset] = (byte) row;
                source[offset + 1] = (byte) row;
                source[offset + 2] = (byte) row;
                source[offset + 3] = (byte) 255;
            }
        }

        byte[] flipped = FrameLayout.flipRows(source, width, height);
        check(flipped[0] == 2 && flipped[1] == 2, "the last row must come first");
        check(flipped[(height - 1) * width * 4] == 0, "the first row must come last");
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                int offset = (row * width + column) * 4;
                check(flipped[offset] == height - 1 - row, "every row must be reversed once");
            }
        }

        byte[] twice = FrameLayout.flipRows(flipped, width, height);
        check(java.util.Arrays.equals(twice, source), "flipping twice must be the identity");

        // A 1x1 frame has nothing to flip and must still round-trip.
        byte[] single = {1, 2, 3, 4};
        check(java.util.Arrays.equals(FrameLayout.flipRows(single, 1, 1), single),
            "a one-pixel frame must survive a flip");

        expect(() -> FrameLayout.flipRows(new byte[4], 2, 2), "a short frame must be refused");
        expect(() -> FrameLayout.flipRows(new byte[4], 0, 1), "a zero width must be refused");
        expect(() -> FrameLayout.frameBytes(0, 5), "a zero height must be refused");

        check(FrameLayout.frameBytes(1920, 1080) == 1920L * 1080L * 4L,
            "a 1920x1080 frame is its pixel count times four");
        check(FrameLayout.frameBytes(1, 1) == 4L, "a 1x1 frame is four bytes");

        String meta = FrameLayout.meta(7L, 123456789L, 1920, 1080);
        check(FrameLayout.field(meta, "sequence") == 7L, "the sequence must survive the meta line");
        check(FrameLayout.field(meta, "capturedNanos") == 123456789L, "the clock must survive");
        check(FrameLayout.field(meta, "width") == 1920L, "the width must survive");
        check(FrameLayout.field(meta, "height") == 1080L, "the height must survive");
        check(meta.contains("format=rgba8-topdown"), "the format must be named, not assumed");
        check(FrameLayout.field(meta, "missing") == -1L, "an absent field is not zero");
        check(FrameLayout.field(meta, "width") != -1L, "zero is a legal width and must not read as absent");
        check(FrameLayout.field("width=0", "width") == 0L, "zero must parse as zero");
        expect(() -> FrameLayout.meta(0L, 0L, 0, 1080), "a zero-width frame has no meta");

        check(FrameRequest.frames("capture 1") == 1, "one frame is one frame");
        check(FrameRequest.frames("capture 30\n") == 30, "a trailing newline is fine");
        check(FrameRequest.frames("capture") == 1, "no count means one frame");
        expect(() -> FrameRequest.frames("capture 0"), "zero frames must be refused");
        expect(() -> FrameRequest.frames("capture 601"), "too many frames must be refused");
        expect(() -> FrameRequest.frames("capture many"), "a bad count must be refused, not guessed");
        expect(() -> FrameRequest.frames("screenshot"), "an unknown action must be refused");
        expect(() -> FrameRequest.frames(""), "an empty request must be refused");
        expect(() -> FrameRequest.frames(null), "a missing request must be refused");
        check(FrameRequest.defaultBody(4).startsWith("capture 4"), "the default body must be valid");
        check(FrameRequest.defaultPath(new java.io.File("/tmp/game")).endsWith("frame.request"),
            "the request path must end in frame.request");

        // The tool writes exactly what the mod parses; if these two drift, a
        // request is silently ignored and looks like a failed capture.
        Path temporary = Files.createTempFile("crafterhunter-frame-test", ".request");
        try {
            Files.writeString(temporary, FrameRequest.defaultBody(2));
            check(FrameRequest.frames(Files.readString(temporary)) == 2,
                "a request written by the shared helper must parse");
        } finally {
            Files.deleteIfExists(temporary);
        }

        System.out.println(
            "Frame layout checks passed: row order, frame size, meta line, request parsing.");
    }

    private static void check(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private static void expect(Runnable action, String message) {
        try {
            action.run();
        } catch (IllegalArgumentException expected) {
            return;
        }
        throw new AssertionError(message);
    }
}

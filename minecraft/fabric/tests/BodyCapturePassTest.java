import com.mojang.blaze3d.framegraph.FrameGraphBuilder;
import com.mojang.blaze3d.resource.GraphicsResourceAllocator;
import com.mojang.blaze3d.resource.ResourceDescriptor;
import dev.crafterhunter.client.BodyCapturePass;
import java.util.ArrayList;
import java.util.Arrays;

/** Real installed frame-graph/resource implementation, CPU scene as the oracle. */
public final class BodyCapturePassTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
    private static final class Scene {
        final FrameGraphBuilder graph = new FrameGraphBuilder();
        final ArrayList<String> events = new ArrayList<>();
        final int[] display = new int[6];
        int[] captured;
        int allocated, freed, live, maximum, passes;
        boolean failDraw;
        Scene() {
            var main = graph.importExternal("main", display);
            var clear = graph.addPass("clear");
            var output = clear.readsAndWrites(main);
            var transientTarget = clear.createsInternal("translucent", new ResourceDescriptor<int[]>() {
                public int[] allocate() { allocated++; maximum = Math.max(maximum, ++live); return new int[1]; }
                public void free(int[] value) { freed++; live--; }
            });
            clear.executes(() -> {
                passes++; events.add(BodyCapturePass.capturing() ? "capture-clear" : "display-clear");
                Arrays.fill(output.get(), 10); // sky
                transientTarget.get()[0] = 1;
            });
            var solid = graph.addPass("solid-features");
            var solidOutput = solid.readsAndWrites(output); solid.reads(transientTarget);
            solid.executes(() -> {
                if (failDraw) throw new IllegalStateException("failed graph");
                display[0] = 20; // terrain
                display[1] = 30; // another player/mob
                display[2] = 40; // background behind local body
                if (!BodyCapturePass.capturing()) display[2] = 50; // only local body draw omitted
                display[3] = 60; // held item
                display[4] = 70; // armor/cape attachment
            });
            var translucent = graph.addPass("translucent");
            translucent.reads(solidOutput); translucent.reads(transientTarget);
            translucent.disableCulling();
            translucent.executes(() -> { display[5] = 80; });
        }
        void run(boolean filtered, boolean failCopy) {
            BodyCapturePass.execute(graph, GraphicsResourceAllocator.UNPOOLED,
                FrameGraphBuilder.Inspector.NONE, filtered, () -> {
                    check(BodyCapturePass.capturing(), "capture flag vanished before copy");
                    events.add("copy"); captured = display.clone();
                    if (failCopy) throw new IllegalStateException("failed copy");
                });
        }
        void assertDisplay() {
            check(Arrays.equals(display, new int[]{20,30,50,60,70,80}), "normal display differs");
            check(!BodyCapturePass.capturing() && !BodyCapturePass.restoring(), "scope leaked");
        }
    }
    public static void main(String[] args) {
        Scene scene = new Scene(); scene.run(true, false);
        check(Arrays.equals(scene.captured, new int[]{20,30,40,60,70,80}), "body duplicated or attachment/background lost");
        scene.assertDisplay();
        check(scene.events.equals(new ArrayList<>(java.util.List.of("capture-clear","copy","display-clear"))), "copy/display order");
        check(scene.passes == 2 && scene.allocated == 2 && scene.freed == 2 && scene.live == 0 && scene.maximum == 1,
            "resource acquisition schedule or bounded lifetime broken");
        Scene ordinary = new Scene(); ordinary.run(false, false); ordinary.assertDisplay();
        check(ordinary.passes == 1 && ordinary.captured == null, "disabled/first-person duplicated graph");
        Scene copyFailure = new Scene();
        try { copyFailure.run(true, true); throw new AssertionError("copy failure swallowed"); }
        catch (IllegalStateException expected) { check(expected.getMessage().equals("failed copy"), "wrong failure"); }
        copyFailure.assertDisplay();
        check(copyFailure.live == 0, "successful graph resources retained by failed copy");
        Scene graphFailure = new Scene(); graphFailure.failDraw = true;
        try { graphFailure.run(true, false); throw new AssertionError("graph failure swallowed"); }
        catch (IllegalStateException expected) { check(expected.getMessage().equals("failed graph"), "wrong failure"); }
        check(graphFailure.passes == 1 && !BodyCapturePass.capturing() && !BodyCapturePass.restoring(),
            "uncertain GPU graph replayed or scope leaked");
        System.out.println("PASS actual 26.2 graph: filtered body, retained attachments/background, display replay, bounded resources and failure scopes");
    }
}

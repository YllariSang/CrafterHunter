package dev.crafterhunter.client;

import com.mojang.blaze3d.framegraph.FrameGraphBuilder;
import com.mojang.blaze3d.resource.GraphicsResourceAllocator;
import java.lang.reflect.Field;
import java.util.BitSet;
import java.util.List;

/** Reuse one prepared 26.2 graph; never run simulation/extraction a second time. */
public final class BodyCapturePass {
    private static boolean capturing, restoring;
    private BodyCapturePass() {}
    public static boolean capturing() { return capturing; }
    public static boolean restoring() { return restoring; }

    // Version-pinned names, validated against the installed classes by the actual-
    // graph test. execute() appends acquisition plans, so it cannot simply be
    // called twice. Reset only this graph's completed resource schedule; do not
    // close/reassign physical resources or reset the prepared feature buffers.
    private static final class Schedule {
        static final Field PASSES = field(FrameGraphBuilder.class, "passes");
        static final Class<?> PASS = passClass();
        static final Field ACQUIRE = field(PASS, "resourcesToAcquire");
        static final Field RELEASE = field(PASS, "resourcesToRelease");
        static Class<?> passClass() {
            try { return Class.forName("com.mojang.blaze3d.framegraph.FrameGraphBuilder$Pass"); }
            catch (ClassNotFoundException e) { throw new IllegalStateException(e); }
        }
        static Field field(Class<?> type, String name) {
            try { Field f = type.getDeclaredField(name); f.setAccessible(true); return f; }
            catch (ReflectiveOperationException e) { throw new IllegalStateException(e); }
        }
        static void reset(FrameGraphBuilder graph) {
            try {
                for (Object pass : (List<?>) PASSES.get(graph)) {
                    ((List<?>) ACQUIRE.get(pass)).clear();
                    ((BitSet) RELEASE.get(pass)).clear();
                }
            } catch (IllegalAccessException e) { throw new IllegalStateException(e); }
        }
    }

    public static void execute(FrameGraphBuilder graph, GraphicsResourceAllocator allocator,
            FrameGraphBuilder.Inspector inspector, boolean filterBody, Runnable capture) {
        if (!filterBody) { graph.execute(allocator, inspector); return; }
        // Resolve the pinned schedule before any filtered GPU work is issued.
        if (Schedule.PASSES == null || Schedule.ACQUIRE == null || Schedule.RELEASE == null)
            throw new IllegalStateException("unsupported frame graph");
        boolean completed = false;
        capturing = true;
        try {
            graph.execute(allocator, inspector);
            completed = true;
            capture.run(); // copies are ordered before display overwrites the target
        } finally {
            capturing = false;
            // If graph execution failed, its resources have uncertain lifetimes.
            // Propagate the game's failure; do not reuse them in a recovery pass.
            if (completed) {
                Schedule.reset(graph);
                restoring = true;
                try { graph.execute(allocator, inspector); }
                finally { restoring = false; }
            }
        }
    }
}

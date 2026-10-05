package dev.crafterhunter.client;

import com.mojang.blaze3d.buffers.GpuBuffer;
import com.mojang.blaze3d.buffers.GpuBufferSlice;
import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.systems.CommandEncoder;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.textures.GpuTexture;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;
import javax.imageio.ImageIO;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.Camera;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import net.minecraft.client.renderer.GameRenderer;
import org.joml.Matrix4f;

/**
 * Reads Minecraft's world colour and its depth from the same world-render frame.
 *
 * <p>Deliberately separate from {@link FrameCapture}, which is the verified
 * colour-transfer path and is left exactly as it was. This one exists to answer a
 * question that colour alone cannot: how far away anything is.
 *
 * <p>Two things make this harder than copying another texture.
 *
 * <p><b>The depth attachment may not be readable at all.</b> Having a depth texture is
 * not the same as being able to copy it to a buffer; plenty of graphics APIs refuse
 * exactly that. Minecraft 26.2 allocates its main target's depth as
 * {@code GpuFormat.D32_FLOAT}, and the OpenGL backend's {@code copyTextureToBuffer}
 * passes whatever format it is given straight to {@code glReadPixels} with no guard —
 * but that is an argument, not a result, so this class records the format it actually
 * saw and the shape of the data it actually got, rather than assuming either.
 *
 * <p><b>Two asynchronous readbacks must be paired by capture identity.</b> Colour and
 * depth are copied by two callbacks that can land in either order, or not at all. So
 * each capture is stamped with an identity at the moment it is <em>issued</em>, each
 * callback reports which identity it belongs to, and nothing is published until both
 * halves of the <em>same</em> identity have arrived. Pairing by completion order would
 * silently produce frames whose colour and depth are from different moments, which is
 * the one mistake that yields plausible numbers instead of an error.
 */
public final class WorldCapture {

    private static final WorldCapture INSTANCE = new WorldCapture();

    /** How often the request file is looked at. */
    private static final long POLL_INTERVAL_NANOS = 250_000_000L;

    /** Refuse an absurd frame rather than trying to allocate it. */
    private static final int MAX_DIMENSION = 8192;

    /**
     * Identity of this writer's lifetime.
     *
     * <p>Sequence numbers restart at one when Minecraft restarts, so a sequence alone
     * cannot tell "the guest restarted" from "the guest is still going". A generation
     * stamped once per process does, and it is what lets a reader refuse a pair that
     * mixes halves from two different runs of the game.
     */
    private final long generation = System.nanoTime();

    public static WorldCapture instance() {
        return INSTANCE;
    }

    private final Path requestFile = FabricLoader.getInstance().getGameDir()
        .resolve("crafterhunter").resolve("world.request");
    private final Path outputDirectory = FabricLoader.getInstance().getGameDir()
        .resolve("crafterhunter").resolve("out");
    private final Path summaryFile = outputDirectory.resolve("world-capture.txt");

    private GpuBuffer colourBuffer;
    private GpuBuffer depthBuffer;
    private int bufferWidth;
    private int bufferHeight;

    private long nextPollNanos;
    private int remaining;
    private int requestedFrames = 1;
    private long captured;
    private String status = "idle";

    // The capture currently in flight, and the identity its halves belong to. A
    // capture is only published once both halves of this identity have landed.
    private long inFlightIdentity;
    private boolean inFlight;
    private String inFlightColourFormat = "?";
    private String inFlightDepthFormat = "?";
    private String inFlightAnchorSource = "?";
    private float inFlightX;
    private float inFlightY;
    private float inFlightZ;
    private float inFlightXRot;
    private float inFlightYRot;
    private float inFlightFov = Float.NaN;
    private float[] inFlightNear = {Float.NaN};
    private float[] inFlightFar = {Float.NaN};
    private float[] inFlightMatrix = new float[16];
    private boolean inFlightMatrixKnown;
    private String inFlightPerspective = "?";
    private long inFlightQueuedNanos;
    private boolean hookSeen;
    private WorldCopyState copyState;
    // Fail closed after uncertain GPU work. Retain this one buffer pair until
    // process exit: no reuse, close, spin wait, or replacement allocation.
    private boolean captureDisabled;

    private WorldCapture() {
    }

    /** Records that the world-render hook fired, so "never hooked" is distinguishable. */
    public void markHookSeen() {
        hookSeen = true;
    }

    public boolean hookSeen() {
        return hookSeen;
    }

    /** One line for the HUD. */
    public String hudLine() {
        return "WORLD: " + status;
    }

    /**
     * Called from the world-render hook at the end of every world frame.
     *
     * @param nowNanos monotonic clock, for ageing the capture
     */
    public void onWorldRendered(long nowNanos) {
        if (captureDisabled) {
            if (nowNanos >= nextSummaryNanos) {
                nextSummaryNanos = nowNanos + SUMMARY_INTERVAL_NANOS;
                writeSummary();
            }
            return;
        }
        if (inFlight && copyState != null && copyState.complete()) {
            publish(nowNanos);
        }

        // A half that never lands must become a report, not an indefinite wait.
        // The first version of this class had no such path and the consequence was
        // observed live: the driver refused the depth copy with
        // GL_INVALID_FRAMEBUFFER_OPERATION, no callback ever fired, and the capture
        // sat in flight forever writing nothing. A stall that reports nothing is
        // indistinguishable from a stall that has not happened yet.
        if (inFlight && copyState != null && !copyState.complete()
                && nowNanos - inFlightQueuedNanos > IN_FLIGHT_TIMEOUT_NANOS) {
            inFlight = false;
            captureDisabled = true;
            remaining = 0;
            status = "capture disabled; buffers retained: incomplete copy after " + (IN_FLIGHT_TIMEOUT_NANOS / 1_000_000L)
                + "ms - the copy was refused (colour=" + colourLanded
                + " depth=" + depthLanded + ")";
            writeSummary();
            return;
        }

        if (remaining == 0 && nowNanos >= nextPollNanos) {
            nextPollNanos = nowNanos + POLL_INTERVAL_NANOS;
            readRequest();
        }

        if (remaining > 0 && !inFlight) {
            queueCapture(nowNanos);
        }

        // The summary is written whether or not a capture was asked for, because
        // "the hook never fired" has to be distinguishable from "nothing was
        // requested". Without this the file only appeared after a successful publish,
        // so the one failure it exists to detect - a hook that never ran - was the
        // one failure it could not report.
        if (nowNanos >= nextSummaryNanos) {
            nextSummaryNanos = nowNanos + SUMMARY_INTERVAL_NANOS;
            writeSummary();
        }
    }

    /** Which half has landed. Separate flags: they may arrive in either order. */
    private volatile boolean colourLanded;
    private volatile boolean depthLanded;

    private long nextSummaryNanos;

    /** How long a capture may stay in flight before it is reported as stalled. */
    private static final long IN_FLIGHT_TIMEOUT_NANOS = 3_000_000_000L;

    /** How often the summary is refreshed when nothing is happening. */
    private static final long SUMMARY_INTERVAL_NANOS = 500_000_000L;

    /**
     * Which attachments a request asked for.
     *
     * <p>Exists because the live run could not tell whether the driver refused the
     * depth copy, the colour copy, or both: both were issued into one submit and the
     * driver reported one error line, which is consistent with either answer. Asking
     * for them separately is the only way to find out, and guessing would have been
     * the difference between fixing the right thing and fixing the wrong thing.
     */
    private enum Mode {
        BOTH,
        COLOUR_ONLY,
        DEPTH_ONLY
    }

    private Mode mode = Mode.BOTH;

    private void readRequest() {
        try {
            if (!Files.exists(requestFile)) {
                return;
            }
            Path claimed = requestFile.resolveSibling("world.claimed-" + java.util.UUID.randomUUID());
            Files.move(requestFile, claimed, java.nio.file.StandardCopyOption.ATOMIC_MOVE);
            String line;
            try {
                line = Files.readString(claimed, StandardCharsets.UTF_8).trim();
            } finally {
                Files.deleteIfExists(claimed);
            }
            if (line.isEmpty()) {
                return;
            }
            String[] parts = line.split("\\s+");
            String verb = parts[0];
            switch (verb) {
                case "capture" -> mode = Mode.BOTH;
                case "colour", "color" -> mode = Mode.COLOUR_ONLY;
                case "depth" -> mode = Mode.DEPTH_ONLY;
                default -> {
                    status = "unknown request: " + line;
                    return;
                }
            }
            int frames = parts.length > 1 ? Integer.parseInt(parts[1]) : 1;
            if (frames < 1 || frames > 64) {
                status = "refused frame count " + frames;
                return;
            }
            requestedFrames = frames;
            remaining = frames;
            status = "requested " + frames + " " + mode;
        } catch (IOException | NumberFormatException problem) {
            status = "request failed: " + problem;
        }
    }

    private void queueCapture(long nowNanos) {
        Minecraft minecraft = Minecraft.getInstance();
        RenderTarget target = minecraft.gameRenderer.mainRenderTarget();
        if (target == null) {
            status = "no main render target";
            remaining = 0;
            return;
        }
        int width = target.width;
        int height = target.height;
        boolean wantColour = mode != Mode.DEPTH_ONLY;
        boolean wantDepth = mode != Mode.COLOUR_ONLY;
        GpuTexture colour = wantColour ? target.getColorTexture() : null;
        GpuTexture depth = wantDepth ? target.getDepthTexture() : null;
        if (width <= 0 || height <= 0 || width > MAX_DIMENSION || height > MAX_DIMENSION
                || (wantColour && colour == null) || (wantDepth && depth == null)) {
            // A missing attachment is reported as itself rather than folded into
            // "unusable target", which would hide which one was absent.
            status = "unusable target " + width + "x" + height
                + " colour=" + (wantColour ? String.valueOf(colour != null) : "not asked")
                + " depth=" + (wantDepth ? String.valueOf(depth != null) : "not asked");
            remaining = 0;
            return;
        }

        // The identity is fixed now, before either copy is issued. Every callback
        // carries this value, and publication requires both halves to match it.
        final long identity = ++lastIdentity;

        // Camera pose and projection are sampled HERE, at issue, not at publish. The
        // camera moves between the two, and a depth buffer paired with the wrong
        // projection reconstructs distances that are confidently wrong.
        recordCamera(minecraft, identity);

        try {
            long colourBytes = (long) width * height * 4L;
            long depthBytes = (long) width * height * 4L;   // D32_FLOAT
            ensureBuffers(minecraft, colourBytes, depthBytes);

            colourLanded = false;
            depthLanded = false;
            inFlightIdentity = identity;
            inFlight = true;
            inFlightQueuedNanos = nowNanos;
            inFlightColourFormat = wantColour ? String.valueOf(colour.getFormat()) : "not requested";
            inFlightDepthFormat = wantDepth ? String.valueOf(depth.getFormat()) : "not requested";
            final WorldCopyState completion = new WorldCopyState(wantColour, wantDepth);
            copyState = completion;

            CommandEncoder encoder = RenderSystem.getDevice().createCommandEncoder();
            // Copies go into one submit, so the first cannot land while the second is
            // still unrecorded - the same pairing problem one level down.
            //
            // A refused copy raises nothing in Java. The live run proved this: the
            // driver returned GL_INVALID_FRAMEBUFFER_OPERATION and the callback simply
            // never fired, so the only honest signal that a copy did not happen is
            // that its flag never arrives. That is why the in-flight timeout exists.
            if (wantColour) {
                encoder.copyTextureToBuffer(colour, colourBuffer, 0L,
                    () -> {
                        completion.colourCompleted();
                        if (inFlightIdentity == identity) colourLanded = true;
                    }, 0, 0, 0, width, height);
            }
            if (wantDepth) {
                encoder.copyTextureToBuffer(depth, depthBuffer, 0L,
                    () -> {
                        completion.depthCompleted();
                        if (inFlightIdentity == identity) depthLanded = true;
                    }, 0, 0, 0, width, height);
            }
            encoder.submit();
            status = "copying " + width + "x" + height + " " + mode;
        } catch (RuntimeException problem) {
            inFlight = false;
            captureDisabled = true;
            remaining = 0;
            status = "capture disabled; buffers retained after copy/setup failure: " + problem;
        }
    }

    private long lastIdentity;
    private long colourLength;
    private long depthLength;

    private void ensureBuffers(Minecraft minecraft, long colourBytes, long depthBytes) {
        if (colourBuffer != null && depthBuffer != null
                && bufferWidth == minecraft.gameRenderer.mainRenderTarget().width
                && bufferHeight == minecraft.gameRenderer.mainRenderTarget().height) {
            return;
        }
        closeBuffers();
        var device = RenderSystem.getDevice();
        colourBuffer = device.createBuffer(() -> "crafterhunter-world-colour",
            GpuBuffer.USAGE_COPY_DST | GpuBuffer.USAGE_MAP_READ, colourBytes);
        depthBuffer = device.createBuffer(() -> "crafterhunter-world-depth",
            GpuBuffer.USAGE_COPY_DST | GpuBuffer.USAGE_MAP_READ, depthBytes);
        colourLength = colourBytes;
        depthLength = depthBytes;
        bufferWidth = minecraft.gameRenderer.mainRenderTarget().width;
        bufferHeight = minecraft.gameRenderer.mainRenderTarget().height;
    }

    private void closeBuffers() {
        if (colourBuffer != null) {
            colourBuffer.close();
            colourBuffer = null;
        }
        if (depthBuffer != null) {
            depthBuffer.close();
            depthBuffer = null;
        }
        bufferWidth = 0;
        bufferHeight = 0;
    }

    /**
     * Samples the camera pose and projection, recording where each value came from.
     *
     * <p>The camera's own render state is preferred because it already accounts for
     * third-person pullback; the player's eye position does not, and using it in third
     * person would put the anchor in the wrong place. Which one was used is recorded,
     * because an anchor of unknown provenance is worse than none.
     */
    private void recordCamera(Minecraft minecraft, long identity) {
        inFlightAnchorSource = "none";
        inFlightMatrixKnown = false;
        inFlightNear[0] = Float.NaN;
        inFlightFar[0] = Float.NaN;

        // Perspective is recorded rather than assumed, because the first-person hand
        // is drawn inside the world pass and no hook here excludes it.
        try {
            inFlightPerspective = String.valueOf(minecraft.options.getCameraType());
        } catch (RuntimeException problem) {
            inFlightPerspective = "unavailable: " + problem;
        }

        Camera camera = minecraft.gameRenderer.mainCamera();
        if (camera != null) {
            try {
                CameraRenderState state = new CameraRenderState();
                camera.extractRenderState(state, 1.0f);
                if (state.initialized && state.pos != null) {
                    inFlightX = (float) state.pos.x;
                    inFlightY = (float) state.pos.y;
                    inFlightZ = (float) state.pos.z;
                    inFlightXRot = state.xRot;
                    inFlightYRot = state.yRot;
                    inFlightAnchorSource = "camera-render-state";
                }
            } catch (RuntimeException | LinkageError problem) {
                inFlightAnchorSource = "camera-render-state failed: " + problem;
            }
        }

        if ("none".equals(inFlightAnchorSource) && minecraft.player != null) {
            var eye = minecraft.player.getEyePosition();
            inFlightX = (float) eye.x;
            inFlightY = (float) eye.y;
            inFlightZ = (float) eye.z;
            inFlightXRot = minecraft.player.getXRot();
            inFlightYRot = minecraft.player.getYRot();
            // No fov from the player: LocalPlayer has no accessor for it, and the
            // projection below is the authoritative one anyway.
            inFlightAnchorSource = "player-eye";
        }

        // The projection the renderer actually used, if it can be reached. Read from
        // the game's own Projection rather than rebuilt from fov, because a rebuilt
        // one is a guess about near and far that would be indistinguishable from a
        // measurement in the output.
        try {
            var projection = ProjectionAccess.levelProjection(minecraft.gameRenderer);
            if (projection != null) {
                inFlightNear[0] = projection.zNear();
                inFlightFar[0] = projection.zFar();
                inFlightFov = projection.fov();
                Matrix4f matrix = projection.getMatrix(new Matrix4f());
                matrix.get(inFlightMatrix);
                inFlightMatrixKnown = true;
            }
        } catch (RuntimeException | LinkageError problem) {
            inFlightNear[0] = Float.NaN;
            inFlightFar[0] = Float.NaN;
        }
    }

    private void publish(long nowNanos) {
        int width = bufferWidth;
        int height = bufferHeight;
        long identity = inFlightIdentity;
        boolean wantColour = mode != Mode.DEPTH_ONLY;
        boolean wantDepth = mode != Mode.COLOUR_ONLY;

        // The pairing assertion, stated rather than assumed: every half asked for
        // must belong to the capture being published. A mode that asked for one
        // attachment is not a failure of pairing.
        boolean satisfied = (!wantColour || colourLanded) && (!wantDepth || depthLanded);
        if (!satisfied || identity != inFlightIdentity) {
            status = "halves did not pair (colour=" + colourLanded
                + " depth=" + depthLanded + " wanted " + mode + ")";
            inFlight = false;
            remaining = 0;
            writeSummary();
            return;
        }

        try {
            byte[] colour = new byte[width * height * 4];
            byte[] depth = new byte[width * height * 4];
            if (wantColour) {
                readInto(colourBuffer, colour);
                lastColourBytes = colourLength;
            }
            if (wantDepth) {
                readInto(depthBuffer, depth);
                lastDepthBytes = depthLength;
            }

            Files.createDirectories(outputDirectory);
            // Depth is raw, not PNG: it is data, and re-encoding it would make the
            // row layout and the exact values unrecoverable.
            if (wantDepth) {
                Files.write(outputDirectory.resolve("world-depth.f32"), depth);
            }
            if (wantColour) {
                Files.write(outputDirectory.resolve("world-colour.rgba"), colour);
                writePng(colour, width, height);
            }

            String metadata = describe(width, height, identity, nowNanos, colour, depth, wantDepth);
            Files.writeString(outputDirectory.resolve("world-capture.meta"), metadata,
                StandardCharsets.UTF_8);

            captured++;
            remaining--;
            inFlight = false;
            status = "captured " + captured + "/" + requestedFrames + " " + mode;
            writeSummary();
        } catch (IOException | RuntimeException problem) {
            inFlight = false;
            remaining = 0;
            status = "publish failed: " + problem;
            writeSummary();
        }
    }

    /** Records what was actually observed, including the things that are unknown. */
    private String describe(int width, int height, long identity, long nowNanos,
            byte[] colour, byte[] depth, boolean haveDepth) {
        StringBuilder text = new StringBuilder(1024);
        text.append("# Paired world capture. Recorded, not assumed.\n");
        text.append("generation=").append(generation).append('\n');
        text.append("identity=").append(identity).append('\n');
        text.append("sequence=").append(captured).append('\n');
        text.append("mode=").append(mode).append('\n');
        text.append("capturedAtNanos=").append(nowNanos).append('\n');
        text.append("ageMillis=").append((nowNanos - inFlightQueuedNanos) / 1_000_000.0).append('\n');
        text.append("width=").append(width).append('\n');
        text.append("height=").append(height).append('\n');
        text.append("colourFormat=").append(inFlightColourFormat).append('\n');
        text.append("depthFormat=").append(inFlightDepthFormat).append('\n');
        text.append("depthBytesPerTexel=4\n");
        text.append("depthTightStride=").append(width * 4).append('\n');
        // If these exceed width*4 the rows are padded, and a tight-stride read would
        // shear every row but the first.
        text.append("depthBufferBytes=").append(lastDepthBytes).append('\n');
        text.append("colourBufferBytes=").append(lastColourBytes).append('\n');
        text.append("depthRowsPadded=")
            .append(haveDepth && lastDepthBytes > (long) width * height * 4L).append('\n');
        text.append("depthRowOrder=topdown\n");
        text.append("depthConvention=opengl-window-depth\n");
        text.append("anchorSource=").append(inFlightAnchorSource).append('\n');
        text.append(String.format(Locale.ROOT, "anchor=%.4f %.4f %.4f%n",
            inFlightX, inFlightY, inFlightZ));
        text.append(String.format(Locale.ROOT, "anchorRot=%.3f %.3f%n",
            inFlightXRot, inFlightYRot));
        text.append("perspective=").append(inFlightPerspective).append('\n');
        text.append("fov=").append(inFlightFov).append('\n');
        text.append("zNear=").append(inFlightNear[0]).append('\n');
        text.append("zFar=").append(inFlightFar[0]).append('\n');
        text.append("projectionMatrixKnown=").append(inFlightMatrixKnown).append('\n');
        if (inFlightMatrixKnown) {
            text.append("projectionMatrix=");
            for (int i = 0; i < 16; i++) {
                text.append(inFlightMatrix[i]).append(i == 15 ? '\n' : ' ');
            }
        }

        if (!haveDepth) {
            // No depth was asked for, so no depth statistics. Reporting statistics of
            // an untouched zero buffer would be worse than reporting nothing: it looks
            // like a measurement of a flat depth buffer.
            text.append("depthStats=not requested\n");
            text.append("colourNonZero=").append(countNonZero(colour)).append('\n');
            return text.toString();
        }

        // What the depth actually contained, so "the readback worked" is a claim about
        // data rather than about the absence of an exception.
        FloatSummary summary = FloatSummary.of(depth, width, height);
        text.append("depthStats=").append(mode).append('\n');
        text.append("depthMin=").append(summary.min).append('\n');
        text.append("depthMax=").append(summary.max).append('\n');
        text.append("depthMean=").append(String.format(Locale.ROOT, "%.6f", summary.mean)).append('\n');
        text.append("depthDistinctApprox=").append(summary.distinctApprox).append('\n');
        text.append("depthCornersTL_TR_BL_BR=").append(summary.corners).append('\n');
        text.append("depthNonFinite=").append(summary.nonFinite).append('\n');
        text.append("depthFirstRowDistinct=").append(summary.firstRowDistinct).append('\n');
        text.append("depthLastRowDistinct=").append(summary.lastRowDistinct).append('\n');
        text.append("colourNonZero=").append(countNonZero(colour)).append('\n');
        return text.toString();
    }

    /**
     * What the depth buffer actually contained.
     *
     * <p>Recorded rather than assumed, because "the readback did not throw" is not
     * evidence that it returned depth: a driver that quietly returned colour, or zeros,
     * or a differently-sized surface would all copy without complaint. These numbers are
     * what make the claim checkable.
     */
    private record FloatSummary(float min, float max, double mean, int distinctApprox,
            int nonFinite, String corners, int firstRowDistinct, int lastRowDistinct) {

        static FloatSummary of(byte[] raw, int width, int height) {
            if (raw == null || width <= 0 || height <= 0) {
                return new FloatSummary(Float.NaN, Float.NaN, Double.NaN, 0, 0, "n/a", 0, 0);
            }
            ByteBuffer buffer = ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN);
            float min = Float.POSITIVE_INFINITY;
            float max = Float.NEGATIVE_INFINITY;
            double sum = 0.0;
            int distinct = 0;
            int nonFinite = 0;
            float last = Float.NaN;
            int firstRowDistinct = 0;
            int lastRowDistinct = 0;
            float firstRowLast = Float.NaN;
            float lastRowLast = Float.NaN;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    float value = buffer.getFloat((y * width + x) * 4);
                    if (!Float.isFinite(value)) {
                        nonFinite++;
                        continue;
                    }
                    min = Math.min(min, value);
                    max = Math.max(max, value);
                    sum += value;
                    if (Float.isNaN(last) || Math.abs(value - last) > 1e-6f) {
                        distinct++;
                        last = value;
                    }
                    if (y == 0 && (Float.isNaN(firstRowLast)
                            || Math.abs(value - firstRowLast) > 1e-6f)) {
                        firstRowDistinct++;
                        firstRowLast = value;
                    }
                    if (y == height - 1 && (Float.isNaN(lastRowLast)
                            || Math.abs(value - lastRowLast) > 1e-6f)) {
                        lastRowDistinct++;
                        lastRowLast = value;
                    }
                }
            }
            int usable = width * height - nonFinite;
            String corners = String.format(Locale.ROOT, "%.6f %.6f %.6f %.6f",
                at(raw, width, 0, 0), at(raw, width, 0, width - 1),
                at(raw, width, height - 1, 0), at(raw, width, height - 1, width - 1));
            return new FloatSummary(
                usable == 0 ? Float.NaN : min,
                usable == 0 ? Float.NaN : max,
                usable == 0 ? Double.NaN : sum / usable,
                distinct, nonFinite, corners, firstRowDistinct, lastRowDistinct);
        }

        private static float at(byte[] raw, int width, int y, int x) {
            int offset = (y * width + x) * 4;
            if (offset < 0 || offset + 4 > raw.length) {
                return Float.NaN;
            }
            return ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN).getFloat(offset);
        }
    }

    private static int countNonZero(byte[] data) {
        int count = 0;
        for (byte value : data) {
            if (value != 0) {
                count++;
            }
        }
        return count;
    }

    /**
     * The number of bytes the driver actually gave us for one attachment.
     *
     * <p>Recorded because the row layout is a question, not a detail. A tight row is
     * {@code width * 4} bytes; a driver may pad rows to an alignment, and if it does then
     * reading at a tight stride shears every row after the first. The colour path
     * assumes tight rows and its output looks right, which is evidence but not proof, so
     * depth is measured the same way and the number written down.
     */
    private long lastColourBytes;
    private long lastDepthBytes;

    private void readInto(GpuBuffer buffer, byte[] destination) {
        try (GpuBufferSlice.MappedView view = buffer.map(true, false)) {
            ByteBuffer data = view.data();
            int available = data.remaining();
            if (available < destination.length) {
                throw new IllegalStateException("readback short: " + available + " of "
                    + destination.length + " bytes");
            }
            data.get(destination, 0, destination.length);
        }
    }

    private void writePng(byte[] colour, int width, int height) throws IOException {
        BufferedImage image = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = (y * width + x) * 4;
                image.setRGB(x, y, ((colour[offset] & 0xFF) << 16)
                    | ((colour[offset + 1] & 0xFF) << 8) | (colour[offset + 2] & 0xFF));
            }
        }
        ImageIO.write(image, "png", outputDirectory.resolve("world-colour.png").toFile());
    }

    private void writeSummary() {
        try {
            Files.createDirectories(outputDirectory);
            StringBuilder text = new StringBuilder();
            text.append("captured=").append(captured).append('\n');
            text.append("requested=").append(requestedFrames).append('\n');
            text.append("mode=").append(mode).append('\n');
            text.append("status=").append(status).append('\n');
            text.append("hookSeen=").append(hookSeen).append('\n');
            text.append("depthFormat=").append(inFlightDepthFormat).append('\n');
            text.append("colourFormat=").append(inFlightColourFormat).append('\n');
            // Which half has landed. The live failure was a copy the driver refused
            // without raising anything in Java, so these two flags are the only honest
            // signal that a copy did not happen - and they are worth publishing
            // precisely while nothing has landed.
            text.append("colourLanded=").append(colourLanded).append('\n');
            text.append("depthLanded=").append(depthLanded).append('\n');
            text.append("inFlight=").append(inFlight).append('\n');
            text.append("remaining=").append(remaining).append('\n');
            text.append("inFlightMillis=").append(inFlight
                ? (System.nanoTime() - inFlightQueuedNanos) / 1_000_000L : -1).append('\n');
            Files.writeString(summaryFile, text.toString(), StandardCharsets.UTF_8);
        } catch (IOException ignored) {
            // Instrumentation must never break the frame it measures.
        }
    }
}

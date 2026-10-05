package dev.crafterhunter.client;

import com.mojang.blaze3d.buffers.GpuBuffer;
import com.mojang.blaze3d.buffers.GpuBufferSlice;
import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.systems.CommandEncoder;
import com.mojang.blaze3d.systems.GpuDevice;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.textures.GpuTexture;
import java.awt.image.BufferedImage;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;
import javax.imageio.ImageIO;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.Minecraft;

/**
 * Reads Minecraft's rendered frame off the GPU so it can be composited into
 * MHW's renderer.
 *
 * Nothing here runs unless a request file appears. When one does, the copy is
 * queued through the game's own encoder — a texture-to-buffer copy with a
 * completion callback — and read back on a later frame from a mapped buffer, so
 * the render thread never blocks on a full-frame download. This is the cheapest
 * honest test of the whole idea: if a frame cannot be read here, no amount of
 * work on the native side helps.
 *
 * The published frame is {@link FrameLayout}'s contract: RGBA8, top-down, with
 * a meta line naming its sequence. The PNG beside it is only for a person to
 * look at, which is how this gets accepted before anything composites it.
 */
public final class FrameCapture {
    private static final FrameCapture INSTANCE = new FrameCapture();

    /** How often the request file is looked at. Reading a file per frame is not free. */
    private static final long POLL_INTERVAL_NANOS = 250_000_000L;

    /** Refuse an absurd frame rather than trying to allocate it. */
    private static final int MAX_DIMENSION = 8192;

    public static FrameCapture instance() {
        return INSTANCE;
    }

    private final Path requestFile =
        FabricLoader.getInstance().getGameDir().resolve("crafterhunter").resolve("frame.request");
    private final Path sharedDirectory = Path.of("/dev/shm/crafterhunter");
    private final Path sharedFrame = sharedDirectory.resolve("frame.rgba");
    private final Path sharedMeta = sharedDirectory.resolve("frame.meta");
    private final Path outputDirectory =
        FabricLoader.getInstance().getGameDir().resolve("crafterhunter").resolve("out");
    private final Path summaryFile = outputDirectory.resolve("capture.txt");

    private GpuBuffer buffer;
    private int bufferWidth;
    private int bufferHeight;

    private long nextPollNanos;
    private long copyQueuedNanos;
    private boolean hookSeen;

    /**
     * Two flags, because "the copy is in flight" and "the copy has landed" are
     * different facts, and the transition between them is a state machine worth
     * testing on its own. One flag conflating them shipped once: publication
     * waited for a copy still being written and the request was re-queued
     * forever, so a capture was accepted, reported success, and produced
     * nothing. {@link FrameCopyState} is that machine, checked outside the game.
     */
    private volatile boolean copyInFlight;
    private volatile boolean copyReady;

    private int remaining;
    private int requestedFrames = 1;
    private long captured;
    private long totalMapNanos;
    private long totalPublishNanos;
    private String status = "idle";
    private int lastWidth;
    private int lastHeight;

    /** Called from the render thread at the end of every frame. */
    public void onRenderedFrame(long nowNanos) {
        if (copyReady) {
            copyReady = false;
            publish(nowNanos);
        }

        if (remaining == 0 && nowNanos >= nextPollNanos) {
            nextPollNanos = nowNanos + POLL_INTERVAL_NANOS;
            readRequest();
        }

        if (remaining > 0 && !copyInFlight && !copyReady) {
            queueCopy(nowNanos);
        }
    }

    /**
     * One line for the HUD: what the last capture did.
     *
     * A failure is shown as itself rather than as "idle", because idle and
     * broken look identical from outside and this is the only thing visible
     * without a log file.
     */
    public String hudLine() {
        if (captured == 0) {
            return status.equals("idle") ? "FRAME: idle" : "FRAME: " + status;
        }
        return String.format(
            Locale.ROOT,
            "FRAME: %dx%d | %d frames | map %.2fms publish %.2fms | %s",
            lastWidth,
            lastHeight,
            captured,
            totalMapNanos / 1_000_000.0 / captured,
            totalPublishNanos / 1_000_000.0 / captured,
            status);
    }

    private void readRequest() {
        if (!Files.isRegularFile(requestFile)) {
            return;
        }

        String request;
        try {
            request = Files.readString(requestFile).trim();
            // Deleted before the copy runs, so a request means exactly one run
            // even if the game is busy and the capture has to wait.
            Files.delete(requestFile);
        } catch (IOException exception) {
            status = "request unreadable: " + exception.getMessage();
            return;
        }

        int frames;
        try {
            frames = FrameRequest.frames(request);
        } catch (IllegalArgumentException exception) {
            status = "request refused: " + exception.getMessage();
            return;
        }

        requestedFrames = frames;
        remaining = frames;
        captured = 0;
        totalMapNanos = 0;
        totalPublishNanos = 0;
        lastWidth = 0;
        lastHeight = 0;
        status = "queued " + frames;
        // Written before the copy so "requested but never captured" is
        // distinguishable from "never polled at all" after the fact.
        writeSummary("accepted");
    }

    private void queueCopy(long nowNanos) {
        if (!hookSeen) {
            hookSeen = true;
            System.out.println("[CrafterHunter] Frame hook is live on the render thread");
        }

        RenderTarget target;
        try {
            target = Minecraft.getInstance().gameRenderer.mainRenderTarget();
        } catch (RuntimeException exception) {
            status = "no main render target: " + exception;
            remaining = 0;
            return;
        }
        if (target == null) {
            status = "no main render target";
            remaining = 0;
            return;
        }

        int width = target.width;
        int height = target.height;
        GpuTexture color = target.getColorTexture();
        if (width <= 0 || height <= 0 || width > MAX_DIMENSION || height > MAX_DIMENSION || color == null) {
            status = "unusable target " + width + "x" + height;
            remaining = 0;
            return;
        }

        long bytes = FrameLayout.frameBytes(width, height);
        try {
            GpuDevice device = RenderSystem.getDevice();
            if (buffer == null || bufferWidth != width || bufferHeight != height) {
                if (buffer != null) {
                    buffer.close();
                }
                buffer = device.createBuffer(
                    () -> "crafterhunter-frame",
                    GpuBuffer.USAGE_COPY_DST | GpuBuffer.USAGE_MAP_READ,
                    bytes);
                bufferWidth = width;
                bufferHeight = height;
            }

            CommandEncoder encoder = device.createCommandEncoder();
            copyInFlight = true;
            copyQueuedNanos = nowNanos;
            // The callback is what makes the buffer readable: it fires when the
            // copy has actually landed, which is why publication waits for it
            // instead of assuming the submit was enough.
            encoder.copyTextureToBuffer(
                color,
                buffer,
                0L,
                () -> {
                    copyInFlight = false;
                    copyReady = true;
                },
                0,
                0,
                0,
                width,
                height);
            encoder.submit();
            lastWidth = width;
            lastHeight = height;
        } catch (RuntimeException exception) {
            copyInFlight = false;
            copyReady = false;
            remaining = 0;
            status = "copy failed: " + exception;
            System.out.println("[CrafterHunter] Frame copy failed: " + exception);
            writeSummary(null);
        }
    }

    private void publish(long nowNanos) {
        // Every outcome writes the summary file, including the failures: a
        // capture that silently produces nothing is indistinguishable from a
        // capture that was never requested, and only one of those is a bug in
        // the GPU path.
        try (GpuBufferSlice.MappedView view = buffer.map(true, false)) {
            ByteBuffer data = view.data();
            int stride = bufferWidth * 4;
            byte[] raw = new byte[stride * bufferHeight];
            int available = data.remaining();
            if (available < raw.length) {
                remaining = 0;
                copyInFlight = false;
                status = "readback short: " + available + " of " + raw.length + " bytes";
                writeSummary(null);
                return;
            }

            data.get(raw);
            totalMapNanos += nowNanos - copyQueuedNanos;
            byte[] flipped = FrameLayout.flipRows(raw, bufferWidth, bufferHeight);

            // The raw frame first: it is what the native side will consume, so
            // a PNG writer that fails must not cost us the measurement.
            writeShared(flipped, captured + 1 == requestedFrames);
            writePng(flipped, captured == 0);
            captured++;
            remaining--;
            status = remaining > 0 ? "capturing" : "done";
        } catch (RuntimeException | IOException exception) {
            remaining = 0;
            copyInFlight = false;
            copyReady = false;
            status = "readback failed: " + exception;
            System.out.println("[CrafterHunter] Frame readback failed: " + exception);
        }

        writeSummary(null);
    }

    private void writePng(byte[] pixels, boolean alsoWriteFile) throws IOException {
        if (!alsoWriteFile) {
            return;
        }
        Files.createDirectories(outputDirectory);
        BufferedImage image =
            new BufferedImage(bufferWidth, bufferHeight, BufferedImage.TYPE_INT_ARGB);
        for (int y = 0; y < bufferHeight; y++) {
            for (int x = 0; x < bufferWidth; x++) {
                int offset = (y * bufferWidth + x) * 4;
                int alpha = pixels[offset] & 0xFF;
                image.setRGB(
                    x,
                    y,
                    ((alpha << 24) | ((pixels[offset + 1] & 0xFF) << 16)
                        | ((pixels[offset + 2] & 0xFF) << 8) | (pixels[offset + 3] & 0xFF)));
            }
        }
        ImageIO.write(image, "png", outputDirectory.resolve("frame.png").toFile());
    }

    private void writeShared(byte[] pixels, boolean alsoMeta) throws IOException {
        Files.createDirectories(sharedDirectory);
        Files.write(sharedFrame, pixels);
        if (alsoMeta) {
            Files.writeString(
                sharedMeta,
                FrameLayout.meta(captured + 1, System.nanoTime(), bufferWidth, bufferHeight),
                StandardCharsets.UTF_8);
        }
    }

    /** Writes the summary a person reads to accept or reject the capture. */
    public void writeSummary(String note) {
        try {
            Files.createDirectories(outputDirectory);
            System.out.println("[CrafterHunter] frame summary: " + note + " " + status
                + " " + captured + "/" + requestedFrames + " " + lastWidth + "x" + lastHeight);
            StringBuilder summary = new StringBuilder();
            summary.append("captured=").append(captured).append(System.lineSeparator());
            summary.append("requested=").append(requestedFrames).append(System.lineSeparator());
            summary.append("size=").append(lastWidth).append('x').append(lastHeight)
                .append(System.lineSeparator());
            summary.append("mapMillis=")
                .append(captured == 0 ? 0.0 : totalMapNanos / 1_000_000.0 / captured)
                .append(System.lineSeparator());
            summary.append("publishMillis=")
                .append(captured == 0 ? 0.0 : totalPublishNanos / 1_000_000.0 / captured)
                .append(System.lineSeparator());
            summary.append("status=").append(status).append(System.lineSeparator());
            summary.append("note=").append(note == null ? "" : note).append(System.lineSeparator());
            Files.writeString(summaryFile, summary.toString(), StandardCharsets.UTF_8);
        } catch (IOException exception) {
            System.out.println("[CrafterHunter] frame summary failed: " + exception);
        }
    }
}

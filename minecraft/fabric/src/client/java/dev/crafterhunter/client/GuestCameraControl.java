package dev.crafterhunter.client;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import org.joml.Matrix4fc;

/** Render-thread-owned opt-in request. F7/F9 cannot form a feedback loop. */
public final class GuestCameraControl {
    private static final Path FLAG = FabricLoader.getInstance().getGameDir()
        .resolve("crafterhunter/guest-control.enabled");
    private static final Path CHANNEL = Path.of("/dev/shm/crafterhunter");
    private static final GuestCameraChannel.Acknowledgement ACK = new GuestCameraChannel.Acknowledgement();
    private static long request, sequence;
    private static boolean oldCamera, oldPlayer, projectionKnown, failed;
    private static final float[] projection = new float[16];
    private GuestCameraControl() { }

    public static boolean requested() { return request != 0; }
    public static void poll() {
        long token = 0;
        try {
            if (Files.size(FLAG) <= 32) token = Long.parseLong(Files.readString(FLAG).strip());
        } catch (IOException | NumberFormatException exception) { /* absent or invalid releases */ }
        if (token < 0) token = 0;
        if (token != request) {
            if (request != 0) {
                if (CameraLink.instance().enabled() != oldCamera) CameraLink.instance().toggle();
                if (PlayerLink.instance().enabled() != oldPlayer) PlayerLink.instance().toggle();
            }
            request = token;
            if (request != 0) {
                oldCamera = CameraLink.instance().enabled(); oldPlayer = PlayerLink.instance().enabled();
                if (oldCamera) CameraLink.instance().toggle();
                if (oldPlayer) PlayerLink.instance().toggle();
            }
        }
        projectionKnown = false;
    }
    public static void release() {
        try { Files.deleteIfExists(FLAG); }
        catch (IOException exception) { org.slf4j.LoggerFactory.getLogger("CrafterHunter").error("Camera release failed", exception); }
        poll();
    }
    public static void recordProjection(Matrix4fc matrix) {
        matrix.get(projection); projectionKnown = true;
    }
    public static void capture(CameraRenderState camera, Matrix4fc view) {
        Minecraft mc = Minecraft.getInstance();
        if (!requested() || failed || !projectionKnown || !camera.initialized || camera.pos == null
                || mc.player == null || mc.getCameraEntity() != mc.player || mc.isPaused()
                || CameraLink.instance().enabled() || PlayerLink.instance().enabled()) return;
        Path temporary = null;
        try {
            int mode = mc.options.getCameraType().isFirstPerson() ? 0
                : mc.options.getCameraType().isMirrored() ? 2 : 1;
            float[] rotation = new float[16]; view.get(rotation);
            byte[] bytes = GuestCameraChannel.encode(WorldCapture.instance().generation(), request,
                ++sequence, mode, new double[] {camera.pos.x, camera.pos.y, camera.pos.z}, rotation, projection);
            Files.createDirectories(CHANNEL);
            temporary = Files.createTempFile(CHANNEL, "camera-", ".pending");
            Files.write(temporary, bytes);
            Files.move(temporary, CHANNEL.resolve("camera.pose"), StandardCopyOption.ATOMIC_MOVE,
                StandardCopyOption.REPLACE_EXISTING);
            temporary = null;
        } catch (IOException | IllegalArgumentException exception) {
            failed = true; // no uncertain retry loop on the rendering thread
            org.slf4j.LoggerFactory.getLogger("CrafterHunter").error("Guest camera export disabled until restart", exception);
        } finally {
            if (temporary != null) try { Files.deleteIfExists(temporary); } catch (IOException ignored) { }
        }
    }
    public static String hudLine() {
        if (!requested()) return "CONTROL: preview | F7/F9: incoming links";
        long now = System.nanoTime(), generation = WorldCapture.instance().generation();
        try {
            Path reply = CHANNEL.resolve("camera.reply");
            if (Files.size(reply) <= 160) ACK.observe(Files.readString(reply), request, generation, now);
        } catch (IOException exception) { /* age the prior acknowledgement out */ }
        return "CONTROL: " + (failed ? "export failed" : ACK.active(request, generation, now)
            ? "Minecraft ACTIVE" : "Minecraft waiting/released") + " | F6: release";
    }
}

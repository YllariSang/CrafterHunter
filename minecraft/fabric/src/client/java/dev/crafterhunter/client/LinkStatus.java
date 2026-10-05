package dev.crafterhunter.client;

import net.fabricmc.loader.api.FabricLoader;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;

/**
 * Writes the link telemetry to a file, so synchronization can be measured from data
 * rather than read off a screenshot.
 *
 * <p>The HUD already shows this, and reading it from a screenshot was the wrong
 * instrument twice over: a composited image cannot tell you which frame it came from,
 * and the HUD sits exactly where Minecraft's own debug screen does. This records the
 * same numbers the HUD draws, plus the camera pose, in a form a tool can sample while a
 * capture runs.
 *
 * <p>Written at a few hertz rather than per frame. The numbers being recorded change on
 * the order of tens of milliseconds, so a higher rate would write more without
 * recording more.
 */
public final class LinkStatus {

    /** Milliseconds between writes. */
    private static final long INTERVAL_MILLIS = 250L;

    private static final Path FILE = FabricLoader.getInstance().getGameDir()
        .resolve("crafterhunter").resolve("out").resolve("link.txt");

    private static long lastWriteMillis = -1;

    private LinkStatus() {
    }

    /**
     * Records the current link state if the interval has elapsed.
     *
     * <p>Failures are swallowed on purpose. This is instrumentation for a person
     * measuring a running game, and a telemetry file that can interrupt rendering would
     * be worse than no telemetry at all.
     */
    public static void maybeWrite() {
        long now = System.currentTimeMillis();
        if (lastWriteMillis >= 0 && now - lastWriteMillis < INTERVAL_MILLIS) {
            return;
        }
        lastWriteMillis = now;
        try {
            write(now);
        } catch (IOException | RuntimeException ignored) {
            // Instrumentation must never be able to break the frame it is measuring.
        }
    }

    private static void write(long now) throws IOException {
        CameraFeed.Diagnostics camera = CameraFeed.diagnostics();
        PlayerFeed.Diagnostics player = PlayerFeed.diagnostics();
        CameraLink.Pose pose = CameraLink.instance().pose();
        PlayerLink.Target proxy = PlayerLink.instance().target();

        StringBuilder text = new StringBuilder(512);
        text.append("wallMillis=").append(now).append('\n');

        // Camera link. `live` and the packet rate come from the packets themselves, so
        // this is the transport's own account of itself rather than an inference.
        text.append("camera.live=").append(camera.live()).append('\n');
        text.append("camera.enabled=").append(CameraLink.instance().enabled()).append('\n');
        text.append("camera.packetsPerSecond=").append(camera.packetsPerSecond()).append('\n');
        text.append("camera.total=").append(camera.total()).append('\n');
        text.append("camera.sequence=").append(camera.sequence()).append('\n');
        text.append("camera.ageMillis=").append(camera.ageMillis()).append('\n');

        // The pose is what alignment is measured against: the same numbers the guest
        // would use to place a guest-geometry box in MHW's world.
        if (pose == null) {
            text.append("camera.pose=none\n");
        } else {
            text.append(String.format(Locale.ROOT,
                "camera.pose=%.4f %.4f %.4f %.3f %.3f %.3f%n",
                pose.x(), pose.y(), pose.z(), pose.yaw(), pose.pitch(), pose.fov()));
        }

        text.append("player.live=").append(player.live()).append('\n');
        text.append("player.enabled=").append(PlayerLink.instance().enabled()).append('\n');
        text.append("player.packetsPerSecond=").append(player.packetsPerSecond()).append('\n');
        text.append("player.ageMillis=").append(player.ageMillis()).append('\n');
        if (proxy == null) {
            text.append("player.proxy=none\n");
        } else {
            text.append(String.format(Locale.ROOT, "player.proxy=%.3f %.3f %.3f%n",
                proxy.x(), proxy.y(), proxy.z()));
        }

        Files.createDirectories(FILE.getParent());
        Files.writeString(FILE, text.toString(), StandardCharsets.UTF_8);
    }
}

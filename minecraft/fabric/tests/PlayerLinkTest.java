package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Headless behavioral checks for the host player proxy; no Minecraft client and
 * no third-party test framework, matching the camera checks.
 */
public final class PlayerLinkTest {
    private static PlayerState sample(double x, double y, double z, float yaw, long time) {
        return new PlayerState(x, y, z, 0, 0, 0, 1, yaw, 1, time);
    }

    private static void near(double actual, double expected) {
        if (Math.abs(actual - expected) > 0.0001) {
            throw new AssertionError("Expected " + expected + ", received " + actual);
        }
    }

    private static void check(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private static IllegalArgumentException rejected(Runnable call) {
        try {
            call.run();
        } catch (IllegalArgumentException expected) {
            return expected;
        }
        throw new AssertionError("Expected the payload to be rejected");
    }

    public static void main(String[] args) {
        // The proxy anchors on the player, never on the host's absolute world
        // position: MHW coordinates are hundreds of metres from Minecraft's.
        Object world = new Object();
        PlayerLink link = new PlayerLink();
        long start = 1_000_000_000L;
        PlayerLink.Target first = link.update(sample(-270.25, -455.0, -116.875, 179, start),
            world, 10, 65, 20, start);
        near(first.x(), 10);
        near(first.y(), 65); // Negative host altitude must not become absolute MC altitude.
        near(first.z(), 20);
        near(first.yaw(), 179);

        // One metre of hunter travel becomes one block, through the same
        // frame-rate-independent filter the camera uses.
        double blend = 1 - Math.exp(-1);
        PlayerLink.Target moved = link.update(
            sample(-268.25, -454.0, -114.875, -179, start),
            world, 999, 999, 999, start + 50_000_000L);
        near(moved.x(), 10 + 2 * blend);
        near(moved.y(), 65 + blend);
        near(moved.z(), 20 + 2 * blend);
        near(moved.yaw(), 179 + 2 * blend); // Cross +/-180 along the short arc.

        PlayerLink slower = new PlayerLink();
        PlayerLink faster = new PlayerLink();
        slower.update(sample(0, 0, 0, 0, start), world, 0, 0, 0, start);
        faster.update(sample(0, 0, 0, 0, start), world, 0, 0, 0, start);
        PlayerState target = sample(10, 0, 0, 0, start);
        PlayerLink.Target slowStep = slower.update(target, world, 0, 0, 0, start + 100_000_000L);
        faster.update(target, world, 0, 0, 0, start + 50_000_000L);
        PlayerLink.Target fastStep = faster.update(target, world, 0, 0, 0, start + 100_000_000L);
        near(slowStep.x(), fastStep.x()); // Smoothing must not depend on frame rate.

        // A single frame that jumps further than the placement threshold is a
        // scene change: re-anchor where the player stands instead of sweeping.
        PlayerLink jumping = new PlayerLink();
        jumping.update(sample(0, 0, 0, 0, start), world, 4, 70, 4, start);
        PlayerLink.Target afterJump = jumping.update(
            sample(400, 0, 0, 0, start), world, 4, 70, 4, start + 50_000_000L);
        near(afterJump.x(), 4);
        near(afterJump.y(), 70);

        // Release paths: each must stop applying the proxy on that call.
        check(link.update(null, world, 0, 0, 0, start + 60_000_000L) == null,
            "Stale feed must release the player");
        check(link.target() == null, "A released proxy must expose no target");
        link.toggle();
        check(link.update(sample(0, 0, 0, 0, start), world, 0, 0, 0, start) == null,
            "Disabled link must leave the player under vanilla control");
        link.toggle();
        link.reset();
        near(link.update(sample(10, 10, 10, 0, start), world, 1, 90, 1, start).y(), 90);
        check(link.update(sample(10, 10, 10, 0, start), null, 0, 0, 0, start) == null,
            "No world means nothing to anchor against");
        near(link.update(sample(10, 10, 10, 0, start), new Object(), 2, 80, 2,
            start + 150_000_000L).y(), 80); // A world change re-anchors.

        // Payload decoding, against the golden bytes pinned in the Rust protocol.
        byte[] payload = ByteBuffer.allocate(28).order(ByteOrder.LITTLE_ENDIAN)
            .putFloat(1).putFloat(2).putFloat(3)
            .putFloat(0).putFloat(0).putFloat(0).putFloat(1)
            .array();
        PlayerState decoded = PlayerState.decode(payload, 42, System.nanoTime());
        near(decoded.x(), 1);
        near(decoded.y(), 2);
        near(decoded.z(), 3);
        near(decoded.minecraftYawDegrees(), -180); // Identity quaternion; +180 wraps to -180.

        rejected(() -> PlayerState.decode(new byte[24], 1, 0));
        rejected(() -> PlayerState.decode(ByteBuffer.allocate(28)
            .order(ByteOrder.LITTLE_ENDIAN)
            .putFloat(Float.NaN).putFloat(0).putFloat(0)
            .putFloat(0).putFloat(0).putFloat(0).putFloat(1)
            .array(), 1, 0));
        rejected(() -> PlayerState.decode(new byte[28], 1, 0)); // Zero quaternion.

        // The player feed ages on its own, independent of the camera feed.
        PlayerFeed.publish(decoded);
        check(PlayerFeed.diagnostics().live(), "Fresh player packet must show LIVE");
        check(PlayerFeed.latestFresh().isPresent(), "Fresh player sample must be readable");
        PlayerFeed.publish(sample(0, 0, 0, 0, System.nanoTime() - 600_000_000L));
        check(PlayerFeed.latestFresh().isEmpty(), "Old player sample must be rejected");
        check(CameraFeed.latestFresh().isEmpty(), "Camera feed must stay independent");
        CameraFeed.publish(new CameraState(0, 0, 0, 0, 0, 0, 1, 60, 1.778f, 0.16f, 15000,
            0, 0, 0, 7, System.nanoTime()));
        check(CameraFeed.diagnostics().live(), "Camera feed must not depend on player traffic");
        check(PlayerFeed.latestFresh().isEmpty(), "Stale player sample must stay stale");
        PlayerFeed.clear();
        check(!PlayerFeed.diagnostics().live(), "Cleared player feed must not show LIVE");
        CameraFeed.clear();

        System.out.println("Player link checks passed: anchor, movement, yaw wrapping, "
            + "jump re-anchor, release, world change, decode, feed independence.");
    }
}

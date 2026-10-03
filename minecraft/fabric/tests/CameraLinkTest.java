package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** Headless behavioral checks; no Minecraft client or third-party test framework. */
public final class CameraLinkTest {
    private static CameraState sample(double x, double y, double z, float yaw, long time) {
        return new CameraState(x, y, z, 0, 0, 0, 1, 60, 1.778f, 0.16f, 15000,
            yaw, 0, 0, 1, time);
    }

    private static void near(double actual, double expected) {
        if (Math.abs(actual - expected) > 0.0001) {
            throw new AssertionError("Expected " + expected + ", received " + actual);
        }
    }

    public static void main(String[] args) {
        Object world = new Object();
        CameraLink link = new CameraLink();
        long start = 1_000_000_000L;
        CameraLink.Pose first = link.update(sample(100, -4, 200, 179, start), world,
            10, 65, 20, start);
        near(first.x(), 10);
        near(first.y(), 65); // Negative MHW altitude must never become absolute MC altitude.
        CameraLink.Pose moved = link.update(sample(102, -3, 204, -179, start), world,
            999, 999, 999, start + 50_000_000L);
        double blend = 1 - Math.exp(-1);
        near(moved.x(), 10 + 2 * blend);
        near(moved.y(), 65 + blend);
        near(moved.z(), 20 + 4 * blend);
        near(moved.yaw(), 179 + 2 * blend); // Cross +/-180 along the short arc.
        CameraLink slower = new CameraLink();
        CameraLink faster = new CameraLink();
        slower.update(sample(0, 0, 0, 0, start), world, 0, 0, 0, start);
        faster.update(sample(0, 0, 0, 0, start), world, 0, 0, 0, start);
        CameraState target = sample(10, 0, 0, 0, start);
        CameraLink.Pose slowPose = slower.update(target, world, 0, 0, 0, start + 100_000_000L);
        faster.update(target, world, 0, 0, 0, start + 50_000_000L);
        CameraLink.Pose fastPose = faster.update(target, world, 0, 0, 0, start + 100_000_000L);
        near(slowPose.x(), fastPose.x()); // Smoothing response must not depend on frame rate.
        if (link.update(null, world, 0, 0, 0, start + 60_000_000L) != null) {
            throw new AssertionError("Stale feed must release the camera");
        }
        near(link.update(sample(1000, 1000, 1000, 0, start), world, 40, 70, 40,
            start + 100_000_000L).y(), 70);
        near(link.update(sample(10, 10, 10, 0, start), new Object(), 20, 80, 20,
            start + 150_000_000L).y(), 80);
        link.toggle();
        if (link.update(sample(0, 0, 0, 0, start), world, 0, 0, 0, start) != null) {
            throw new AssertionError("Disabled link must leave vanilla camera in control");
        }
        link.toggle();
        link.reset();
        near(link.update(sample(10, 10, 10, 0, start), world, 1, 90, 1, start).y(), 90);

        byte[] payload = ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN)
            .putFloat(1).putFloat(2).putFloat(3).putFloat(0).putFloat(0).putFloat(0)
            .putFloat(2).putFloat((float) Math.PI / 3).putFloat(1.778f)
            .putFloat(0.16f).putFloat(15000).array();
        CameraState decoded = CameraState.decode(payload, 42, System.nanoTime());
        near(decoded.quaternionW(), 1);
        near(decoded.verticalFovDegrees(), 60);
        CameraFeed.publish(decoded);
        if (!CameraFeed.diagnostics().live() || CameraFeed.latestFresh().isEmpty()) {
            throw new AssertionError("Fresh packet must show LIVE");
        }
        CameraFeed.publish(sample(0, 0, 0, 0, System.nanoTime() - 600_000_000L));
        if (CameraFeed.latestFresh().isPresent()) throw new AssertionError("Old sample accepted");
        CameraFeed.clear();
        if (CameraFeed.diagnostics().live()) throw new AssertionError("Cleared feed shows LIVE");
        System.out.println("Camera link checks passed: anchor, movement, yaw wrapping, recovery, world change, toggle, decode, freshness.");
    }
}

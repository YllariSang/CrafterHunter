package dev.crafterhunter.client;

/** Render-thread-owned camera anchor and time-based smoothing. No game memory writes. */
public final class CameraLink {
    private static final CameraLink INSTANCE = new CameraLink();
    private boolean enabled = true;
    private CameraState origin;
    private Object world;
    private double anchorX, anchorY, anchorZ;
    private Pose pose;
    private long lastFrame;

    public static CameraLink instance() { return INSTANCE; }

    public boolean enabled() { return enabled; }

    public void toggle() {
        enabled = !enabled;
        reset();
    }

    public void reset() {
        origin = null;
        pose = null;
        lastFrame = 0;
    }

    public Pose update(CameraState sample, Object currentWorld,
                       double vanillaX, double vanillaY, double vanillaZ, long now) {
        if (!enabled || sample == null || currentWorld == null) {
            reset();
            return null;
        }
        if (origin == null || world != currentWorld || now - lastFrame > 500_000_000L) {
            world = currentWorld;
            origin = sample;
            anchorX = vanillaX;
            anchorY = vanillaY;
            anchorZ = vanillaZ;
            pose = new Pose(anchorX, anchorY, anchorZ, sample.minecraftYawDegrees(),
                sample.minecraftPitchDegrees(), sample.verticalFovDegrees());
        }
        Pose target = new Pose(anchorX + sample.x() - origin.x(),
            anchorY + sample.y() - origin.y(), anchorZ + sample.z() - origin.z(),
            sample.minecraftYawDegrees(), sample.minecraftPitchDegrees(), sample.verticalFovDegrees());
        double deltaSeconds = lastFrame == 0 ? 0 : Math.max(0, Math.min(0.1, (now - lastFrame) / 1.0e9));
        double blend = 1 - Math.exp(-deltaSeconds / 0.05);
        pose = new Pose(lerp(pose.x(), target.x(), blend), lerp(pose.y(), target.y(), blend),
            lerp(pose.z(), target.z(), blend), angle(pose.yaw(), target.yaw(), blend),
            (float) lerp(pose.pitch(), target.pitch(), blend), (float) lerp(pose.fov(), target.fov(), blend));
        lastFrame = now;
        return pose;
    }

    public Pose pose() { return enabled ? pose : null; }

    private static double lerp(double from, double to, double blend) {
        return from + (to - from) * blend;
    }

    private static float angle(float from, float to, double blend) {
        double delta = ((to - from) % 360 + 540) % 360 - 180;
        return (float) (from + delta * blend);
    }

    public record Pose(double x, double y, double z, float yaw, float pitch, float fov) { }
}

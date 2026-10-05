package dev.crafterhunter.client;

/**
 * Host player proxy: maps hunter displacement onto the Minecraft player through
 * a relative anchor, exactly as {@link CameraLink} maps the camera.
 *
 * MHW's world sits hundreds of metres from anything in a Minecraft world, so
 * absolute coordinates are never applied: the first fresh sample anchors both
 * the host position and the player's current position, and later samples move
 * the proxy by their difference — one metre of hunter travel becomes one block.
 *
 * This class computes a target and writes nothing. The consumer applies it, and
 * stops the moment this returns null: a stale feed, a world change, or a
 * disabled link releases the player immediately rather than leaving it parked
 * at the last known host position.
 */
public final class PlayerLink {
    /**
     * The same threshold the placement anchor uses. A single frame that moves
     * more than this is a scene change or a teleport, not walking, so the link
     * re-anchors instead of sweeping the player across the world to catch up.
     */
    private static final double MAX_STEP_METRES = 25.0;
    /** A gap longer than this means the consumer stopped asking, not that we stalled. */
    private static final long STALE_NANOS = 500_000_000L;

    private static final PlayerLink INSTANCE = new PlayerLink();
    private boolean enabled = true;
    private PlayerState origin;
    private PlayerState previous;
    private Object world;
    private double anchorX;
    private double anchorY;
    private double anchorZ;
    private Target target;
    private long lastFrame;

    public static PlayerLink instance() {
        return INSTANCE;
    }

    public boolean enabled() {
        return enabled;
    }

    public void toggle() {
        enabled = !enabled;
        reset();
    }

    public void reset() {
        origin = null;
        previous = null;
        target = null;
        lastFrame = 0;
    }

    /**
     * Returns where the player should stand this frame, or null when the proxy
     * must let go. {@code vanillaX/Y/Z} is where the player is now, used only to
     * establish the anchor on the first sample and after any re-anchor.
     */
    public Target update(PlayerState sample, Object currentWorld,
                         double vanillaX, double vanillaY, double vanillaZ, long now) {
        if (!enabled || sample == null || currentWorld == null) {
            reset();
            return null;
        }

        boolean reanchor = origin == null || world != currentWorld || now - lastFrame > STALE_NANOS;
        if (!reanchor && previous != null) {
            double step = Math.sqrt(
                (sample.x() - previous.x()) * (sample.x() - previous.x())
                    + (sample.y() - previous.y()) * (sample.y() - previous.y())
                    + (sample.z() - previous.z()) * (sample.z() - previous.z())
            );
            reanchor = step > MAX_STEP_METRES;
        }

        if (reanchor) {
            world = currentWorld;
            origin = sample;
            previous = sample;
            anchorX = vanillaX;
            anchorY = vanillaY;
            anchorZ = vanillaZ;
            target = new Target(vanillaX, vanillaY, vanillaZ, sample.minecraftYawDegrees());
            lastFrame = now;
            return target;
        }

        Target desired = new Target(
            anchorX + sample.x() - origin.x(),
            anchorY + sample.y() - origin.y(),
            anchorZ + sample.z() - origin.z(),
            sample.minecraftYawDegrees()
        );
        double deltaSeconds = lastFrame == 0 ? 0 : Math.max(0, Math.min(0.1, (now - lastFrame) / 1.0e9));
        double blend = 1 - Math.exp(-deltaSeconds / 0.05);
        target = new Target(
            lerp(target.x(), desired.x(), blend),
            lerp(target.y(), desired.y(), blend),
            lerp(target.z(), desired.z(), blend),
            angle(target.yaw(), desired.yaw(), blend)
        );
        previous = sample;
        lastFrame = now;
        return target;
    }

    /** The last computed target, for display. Null when the proxy has released. */
    public Target target() {
        return target;
    }

    private static double lerp(double from, double to, double blend) {
        return from + (to - from) * blend;
    }

    private static float angle(float from, float to, double blend) {
        double delta = ((to - from) % 360 + 540) % 360 - 180;
        return (float) (from + delta * blend);
    }

    public record Target(double x, double y, double z, float yaw) {
    }
}

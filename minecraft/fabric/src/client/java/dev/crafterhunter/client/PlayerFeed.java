package dev.crafterhunter.client;

import java.util.Optional;

/**
 * Handoff from the UDP receiver thread to Minecraft's render thread for host
 * hunter telemetry.
 *
 * This feed is separate from {@link CameraFeed} on purpose: when the host has no
 * player to sample — a loading screen, an area transition — this stream goes
 * quiet while the camera keeps running, and the proxy must release the player
 * on its own age rather than on the camera's health.
 */
public final class PlayerFeed {
    private static final SampleFeed<PlayerState> FEED = new SampleFeed<>();

    private PlayerFeed() {
    }

    static void publish(PlayerState state) {
        FEED.publish(state);
    }

    public static Optional<PlayerState> latestFresh() {
        return FEED.latestFresh();
    }

    static void clear() {
        FEED.clear();
    }

    public static Diagnostics diagnostics() {
        SampleFeed.Snapshot snapshot = FEED.diagnostics();
        return new Diagnostics(snapshot.live(), snapshot.packetsPerSecond(), snapshot.total(),
            snapshot.sequence(), snapshot.ageMillis());
    }

    public record Diagnostics(boolean live, int packetsPerSecond, long total,
                              String sequence, long ageMillis) {
    }
}

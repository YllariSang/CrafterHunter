package dev.crafterhunter.client;

import java.util.Optional;

/**
 * Lock-free handoff from the UDP receiver thread to Minecraft's render thread
 * for camera telemetry. The freshness and arrival rules live in
 * {@link SampleFeed}, which the player proxy shares.
 */
public final class CameraFeed {
    private static final SampleFeed<CameraState> FEED = new SampleFeed<>();

    private CameraFeed() {
    }

    static void publish(CameraState state) {
        FEED.publish(state);
    }

    public static Optional<CameraState> latestFresh() {
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

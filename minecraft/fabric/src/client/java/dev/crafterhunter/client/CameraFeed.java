package dev.crafterhunter.client;

import java.time.Duration;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicReference;

/** Lock-free handoff from the UDP receiver thread to Minecraft's render thread. */
public final class CameraFeed {
    private static final long MAX_SAMPLE_AGE_NANOS = Duration.ofMillis(500).toNanos();
    private static final AtomicReference<CameraState> LATEST = new AtomicReference<>();

    private CameraFeed() {
    }

    static void publish(CameraState state) {
        LATEST.set(state);
    }

    public static Optional<CameraState> latestFresh() {
        CameraState state = LATEST.get();
        if (state == null) {
            return Optional.empty();
        }

        long age = System.nanoTime() - state.receivedAtNanos();
        if (age < 0L || age > MAX_SAMPLE_AGE_NANOS) {
            LATEST.compareAndSet(state, null);
            return Optional.empty();
        }
        return Optional.of(state);
    }

    static void clear() {
        LATEST.set(null);
    }
}

package dev.crafterhunter.client;

import java.time.Duration;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicReference;
import java.util.ArrayDeque;

/** Lock-free handoff from the UDP receiver thread to Minecraft's render thread. */
public final class CameraFeed {
    private static final long MAX_SAMPLE_AGE_NANOS = Duration.ofMillis(500).toNanos();
    private static final AtomicReference<CameraState> LATEST = new AtomicReference<>();
    private static final ArrayDeque<Long> ARRIVALS = new ArrayDeque<>();
    private static long total;

    private CameraFeed() {
    }

    static synchronized void publish(CameraState state) {
        LATEST.set(state);
        total++;
        ARRIVALS.addLast(state.receivedAtNanos());
        while (ARRIVALS.size() > 256) ARRIVALS.removeFirst();
        prune(state.receivedAtNanos());
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

    public static synchronized Diagnostics diagnostics() {
        long now = System.nanoTime();
        prune(now);
        CameraState state = LATEST.get();
        long age = state == null ? -1 : Math.max(0, now - state.receivedAtNanos()) / 1_000_000;
        return new Diagnostics(age >= 0 && age <= 500, ARRIVALS.size(), total,
            state == null ? "-" : Integer.toUnsignedString(state.sequence()), age);
    }

    private static void prune(long now) {
        while (!ARRIVALS.isEmpty() && now - ARRIVALS.peekFirst() > 1_000_000_000L) {
            ARRIVALS.removeFirst();
        }
    }

    public record Diagnostics(boolean live, int packetsPerSecond, long total,
                              String sequence, long ageMillis) { }
}

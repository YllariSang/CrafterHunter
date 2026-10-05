package dev.crafterhunter.client;

import java.time.Duration;
import java.util.ArrayDeque;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Freshness and arrival tracking for one telemetry stream.
 *
 * The camera and the player each own their own feed, so a stream that goes
 * quiet never keeps the other one looking alive, while both age samples out by
 * exactly the same rule. This class is the rule: it used to live in
 * {@link CameraFeed}, and the player proxy needs identical behaviour rather
 * than a second copy that could drift.
 *
 * Reads stay lock-free for the render thread; only publication and diagnostics
 * synchronize.
 */
final class SampleFeed<T extends TelemetrySample> {
    private static final long MAX_SAMPLE_AGE_NANOS = Duration.ofMillis(500).toNanos();
    private static final int ARRIVAL_WINDOW = 256;
    private static final long DIAGNOSTIC_WINDOW_NANOS = 1_000_000_000L;

    private final AtomicReference<T> latest = new AtomicReference<>();
    private final ArrayDeque<Long> arrivals = new ArrayDeque<>();
    private long total;

    synchronized void publish(T sample) {
        latest.set(sample);
        total++;
        arrivals.addLast(sample.receivedAtNanos());
        while (arrivals.size() > ARRIVAL_WINDOW) {
            arrivals.removeFirst();
        }
        prune(sample.receivedAtNanos());
    }

    Optional<T> latestFresh() {
        T sample = latest.get();
        if (sample == null) {
            return Optional.empty();
        }

        long age = System.nanoTime() - sample.receivedAtNanos();
        if (age < 0L || age > MAX_SAMPLE_AGE_NANOS) {
            latest.compareAndSet(sample, null);
            return Optional.empty();
        }
        return Optional.of(sample);
    }

    void clear() {
        latest.set(null);
    }

    synchronized Snapshot diagnostics() {
        long now = System.nanoTime();
        prune(now);
        T sample = latest.get();
        long age = sample == null ? -1 : Math.max(0, now - sample.receivedAtNanos()) / 1_000_000;
        return new Snapshot(sample != null && age <= 500, arrivals.size(), total,
            sample == null ? "-" : Integer.toUnsignedString(sample.sequence()), age);
    }

    private void prune(long now) {
        while (!arrivals.isEmpty() && now - arrivals.peekFirst() > DIAGNOSTIC_WINDOW_NANOS) {
            arrivals.removeFirst();
        }
    }

    record Snapshot(boolean live, int packetsPerSecond, long total, String sequence,
                    long ageMillis) {
    }
}

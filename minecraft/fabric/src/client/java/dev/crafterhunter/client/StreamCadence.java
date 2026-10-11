package dev.crafterhunter.client;

/**
 * Publish pacing for the live paired stream, and what it actually achieved.
 *
 * <p>The 250 ms floor this replaces was a deliberate choice - a request-paced
 * diagnostic, not a video reader - not a measured limit. Whether the guest can
 * publish a paired snapshot every render tick is unknown, and the only honest way
 * to find out is to remove the floor and let the producer report its own
 * cadence rather than assuming one.
 *
 * <p>The flag file's content is the interval in milliseconds. Zero means "no
 * floor": publish on every render tick that has no copy already in flight.
 * Anything unreadable, negative or larger than {@link #MAX_INTERVAL_MILLIS} falls
 * back to {@link #DEFAULT_INTERVAL_MILLIS}, so a typo cannot silently turn into
 * the fastest setting.
 */
public final class StreamCadence {
    public static final long DEFAULT_INTERVAL_MILLIS = 250L;
    public static final long MAX_INTERVAL_MILLIS = 1000L;

    private StreamCadence() { }

    public static long intervalMillis(String content) {
        if (content == null) return DEFAULT_INTERVAL_MILLIS;
        String trimmed = content.trim();
        if (trimmed.isEmpty()) return DEFAULT_INTERVAL_MILLIS;
        long value;
        try {
            value = Long.parseLong(trimmed);
        } catch (NumberFormatException refused) {
            return DEFAULT_INTERVAL_MILLIS;
        }
        if (value < 0L || value > MAX_INTERVAL_MILLIS) return DEFAULT_INTERVAL_MILLIS;
        return value;
    }

    /**
     * Achieved intervals between publications, in the producer's own clock.
     *
     * <p>The first sample only establishes a baseline; a clock that steps
     * backwards produces no interval rather than a negative one, because a
     * negative duration in a cadence report is worse than a missing sample.
     */
    public static final class Report {
        private long baseline;
        private long count;
        private long min = Long.MAX_VALUE;
        private long max;
        private long total;

        public void record(long nowNanos) {
            if (baseline == 0L) { baseline = nowNanos == 0L ? 1L : nowNanos; return; }
            long delta = nowNanos - baseline;
            baseline = nowNanos;
            if (delta < 0L) return;
            count++;
            total += delta;
            min = Math.min(min, delta);
            max = Math.max(max, delta);
        }

        public long count() { return count; }
        public long minMillis() { return count == 0L ? -1L : min / 1_000_000L; }
        public long maxMillis() { return count == 0L ? -1L : max / 1_000_000L; }
        public long meanMillis() { return count == 0L ? -1L : total / count / 1_000_000L; }

        public String summary() {
            return "streamCadence=n" + count + " min=" + minMillis() + "ms mean=" + meanMillis()
                + "ms max=" + maxMillis() + "ms";
        }
    }
}
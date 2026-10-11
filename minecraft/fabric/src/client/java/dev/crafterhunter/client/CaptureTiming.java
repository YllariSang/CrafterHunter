package dev.crafterhunter.client;

import java.util.Locale;

/** Bounded CPU-clock aggregates. Neither callbacks nor publication imply display. */
public final class CaptureTiming {
    private static final String[] NAMES = {
        "issueToCallback", "callbackToService", "bufferCopy", "metadata",
        "channelPublish", "issueToPublish"
    };
    private final long[] count = new long[NAMES.length];
    private final double[] total = new double[NAMES.length];
    private final long[] min = new long[NAMES.length];
    private final long[] max = new long[NAMES.length];
    private long lastIdentity;

    /** Called only after a complete paired snapshot was successfully published. */
    public boolean record(long identity, long issue, long callback, long service,
            long copyStart, long copyEnd, long metadataStart, long metadataEnd,
            long publishStart, long publishEnd) {
        long[] ordered = {issue, callback, service, copyStart, copyEnd,
            metadataStart, metadataEnd, publishStart, publishEnd};
        for (int i = 1; i < ordered.length; i++) {
            if (ordered[i] - ordered[i - 1] < 0) return false;
        }
        if (identity <= lastIdentity) return false;
        long[] spans = {callback - issue, service - callback, copyEnd - copyStart,
            metadataEnd - metadataStart, publishEnd - publishStart, publishEnd - issue};
        for (int i = 0; i < spans.length; i++) {
            min[i] = count[i] == 0 ? spans[i] : Math.min(min[i], spans[i]);
            max[i] = Math.max(max[i], spans[i]);
            count[i]++;
            total[i] += spans[i];
        }
        lastIdentity = identity;
        return true;
    }

    public String summary(long generation) {
        StringBuilder text = new StringBuilder("captureTimingClock=java-nanoTime\n");
        text.append("captureTimingGeneration=").append(generation).append('\n');
        text.append("captureTimingLastIdentity=").append(lastIdentity).append('\n');
        for (int i = 0; i < NAMES.length; i++) {
            text.append(String.format(Locale.ROOT,
                "captureTiming.%s=n%d min=%.3fms mean=%.3fms max=%.3fms%n",
                NAMES[i], count[i], min[i] / 1e6,
                count[i] == 0 ? 0 : total[i] / count[i] / 1e6, max[i] / 1e6));
        }
        return text.toString();
    }
}

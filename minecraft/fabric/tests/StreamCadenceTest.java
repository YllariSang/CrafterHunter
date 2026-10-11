package dev.crafterhunter.client;

import java.util.List;

/** Headless checks for the stream pacing contract and the achieved-cadence report. */
public final class StreamCadenceTest {
    private static int failures;

    private static void check(boolean ok, String what) {
        if (!ok) { failures++; System.out.println("  FAIL: " + what); }
    }

    private static void eq(long expected, long actual, String what) {
        check(expected == actual, what + " (expected " + expected + ", got " + actual + ")");
    }

    public static void main(String[] args) {
        System.out.println("stream cadence contract");

        // The flag is read as an interval in milliseconds, and zero is the setting
        // that removes the floor. Everything else keeps the old behaviour.
        eq(0L, StreamCadence.intervalMillis("0"), "zero means no floor");
        eq(16L, StreamCadence.intervalMillis("16"), "an explicit interval is honoured");
        eq(250L, StreamCadence.intervalMillis("250\n"), "trailing newline is tolerated");
        eq(60L, StreamCadence.intervalMillis("   60  "), "surrounding space is tolerated");

        // A typo must never become the fastest setting. This is the failure that
        // would be invisible until the machine was on fire.
        eq(250L, StreamCadence.intervalMillis(null), "absent content falls back");
        eq(250L, StreamCadence.intervalMillis(""), "empty content falls back");
        eq(250L, StreamCadence.intervalMillis("   "), "blank content falls back");
        eq(250L, StreamCadence.intervalMillis("every frame"), "prose falls back");
        eq(250L, StreamCadence.intervalMillis("-1"), "a negative interval falls back");
        eq(250L, StreamCadence.intervalMillis("99999999999999999999"), "an absurd integer falls back");
        eq(250L, StreamCadence.intervalMillis("5000"), "beyond the bounded maximum falls back");
        eq(1000L, StreamCadence.intervalMillis("1000"), "the maximum itself is accepted");

        // Reproduce the separate player's original 250ms cap at a 60Hz render
        // clock. The production gate must publish every frame when set to zero.
        for (String content : new String[]{"0", "16", "250", "1000", "baked player preview"}) {
            StreamCadence.Pacer pacer = new StreamCadence.Pacer();
            int publications = 0;
            for (int frame = 0; frame < 60; frame++) {
                long now = 1_000_000_000L + frame * 16_666_667L;
                if (pacer.due(now)) {
                    publications++;
                    pacer.schedule(now, content);
                }
            }
            long expected = switch (content) {
                case "0", "16" -> 60L;
                case "1000" -> 1L;
                default -> 4L;
            };
            eq(expected, publications, "render-clock publications for " + content);
        }
        StreamCadence.Pacer boundary = new StreamCadence.Pacer();
        boundary.schedule(1_000_000_000L, "250");
        check(!boundary.due(1_249_999_999L), "old floor still gates before the boundary");
        check(boundary.due(1_250_000_000L), "publication is due at the boundary");
        boundary.schedule(1_250_000_000L, "0");
        check(boundary.due(1_266_666_667L), "changing to zero samples the next render frame");
        boundary.schedule(1_266_666_667L, null);
        check(!boundary.due(1_283_333_334L), "unreadable/disabled request keeps conservative checks");

        // The first sample only establishes a baseline; nothing is reported as a
        // zero-length interval.
        StreamCadence.Report report = new StreamCadence.Report();
        report.record(1_000_000_000L);
        eq(0L, report.count(), "the first sample establishes a baseline only");
        eq(-1L, report.minMillis(), "one sample reports no minimum");
        eq(-1L, report.meanMillis(), "one sample reports no mean");

        // Three further samples at 16 ms, then one at 64 ms: five publications
        // in total, so four intervals rather than five.
        report.record(1_016_000_000L);
        report.record(1_032_000_000L);
        report.record(1_048_000_000L);
        report.record(1_112_000_000L);
        eq(4L, report.count(), "five publications yield four intervals");
        eq(16L, report.minMillis(), "the minimum interval is reported");
        eq(64L, report.maxMillis(), "the maximum interval is reported");
        eq(28L, report.meanMillis(), "the mean interval is reported");

        // A clock that steps backwards must not manufacture a negative duration: the
        // sample is dropped, the count does not move, and the next interval is
        // measured from the new baseline.
        report.record(1_100_000_000L);
        eq(4L, report.count(), "a backwards step adds no interval");
        check(report.minMillis() >= 0L, "no negative interval is reported");
        report.record(1_116_000_000L);
        eq(5L, report.count(), "the next sample measures from the new baseline");
        eq(16L, report.minMillis(), "and the interval it measures is sane");

        check(report.summary().contains("streamCadence=n5"), "the summary carries the count");
        check(report.summary().contains("mean="), "the summary carries the mean");

        if (failures != 0) {
            System.out.println(failures + " check(s) failed");
            System.exit(1);
        }
        System.out.println("all checks passed");
    }
}

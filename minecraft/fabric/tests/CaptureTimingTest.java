package dev.crafterhunter.client;

public final class CaptureTimingTest {
    private static void check(boolean value, String message) {
        if (!value) throw new AssertionError(message);
    }
    public static void main(String[] args) {
        WorldCopyState pair = new WorldCopyState(true, true);
        pair.depthCompleted(30_000_000);
        check(!pair.complete(), "one callback is not a complete pair");
        try { pair.completedNanos(); throw new AssertionError("incomplete time accepted"); }
        catch (IllegalStateException expected) { }
        pair.colourCompleted(20_000_000);
        pair.depthCompleted(90_000_000);
        check(pair.completedNanos() == 30_000_000, "duplicates cannot extend callback delay");
        WorldCopyState colourOnly = new WorldCopyState(true, false);
        colourOnly.colourCompleted(15_000_000);
        check(colourOnly.completedNanos() == 15_000_000, "unrequested depth ignored");
        WorldCopyState depthOnly = new WorldCopyState(false, true);
        depthOnly.depthCompleted(16_000_000);
        check(depthOnly.completedNanos() == 16_000_000, "unrequested colour ignored");

        CaptureTiming report = new CaptureTiming();
        check(report.record(1, 10_000_000, pair.completedNanos(), 70_000_000,
            71_000_000, 74_000_000, 75_000_000, 79_000_000, 80_000_000, 85_000_000), "ordered sample");
        String first = report.summary(999);
        check(first.contains("captureTimingGeneration=999\n"), "generation provenance");
        check(first.contains("issueToCallback=n1 min=20.000ms mean=20.000ms max=20.000ms"), "copy signal delay");
        check(first.contains("callbackToService=n1 min=40.000ms mean=40.000ms max=40.000ms"), "service scheduling delay");
        check(first.contains("bufferCopy=n1 min=3.000ms mean=3.000ms max=3.000ms"), "CPU buffer work");
        check(first.contains("metadata=n1 min=4.000ms mean=4.000ms max=4.000ms"), "metadata work");
        check(first.contains("channelPublish=n1 min=5.000ms mean=5.000ms max=5.000ms"), "channel work");
        check(first.contains("issueToPublish=n1 min=75.000ms mean=75.000ms max=75.000ms"), "total includes intervening work");
        check(!report.record(2, 10, 30, 20, 40, 50, 60, 70, 80, 90), "backward stage omitted");
        check(!report.record(1, 10, 20, 30, 40, 50, 60, 70, 80, 90), "duplicate identity omitted");
        check(report.summary(999).equals(first), "refusals leave report unchanged");
        check(report.record(2, 0, 0, 0, 0, 0, 0, 0, 0, 0), "zero duration is valid");
        check(report.summary(999).contains("issueToCallback=n2 min=0.000ms mean=10.000ms max=20.000ms"), "aggregate bounded state");
        System.out.println("PASS: callback/service separation, duplicates, refusals and local CPU stages");
    }
}

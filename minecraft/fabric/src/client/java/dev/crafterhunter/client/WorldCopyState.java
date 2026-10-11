package dev.crafterhunter.client;

/** One capture's completion signals. Completion is not proof of valid pixels. */
public final class WorldCopyState {
    private final boolean wantColour;
    private final boolean wantDepth;
    private boolean colour;
    private boolean depth;
    private long colourNanos, depthNanos;

    public WorldCopyState(boolean wantColour, boolean wantDepth) {
        if (!wantColour && !wantDepth) throw new IllegalArgumentException("No attachment requested");
        this.wantColour = wantColour;
        this.wantDepth = wantDepth;
    }

    public void colourCompleted() { colourCompleted(System.nanoTime()); }
    public void depthCompleted() { depthCompleted(System.nanoTime()); }
    public synchronized void colourCompleted(long nowNanos) {
        if (!colour) { colourNanos = nowNanos; colour = true; }
    }
    public synchronized void depthCompleted(long nowNanos) {
        if (!depth) { depthNanos = nowNanos; depth = true; }
    }
    public synchronized boolean complete() {
        return (!wantColour || colour) && (!wantDepth || depth);
    }
    public synchronized long completedNanos() {
        if (!complete()) throw new IllegalStateException("Incomplete capture");
        if (!wantColour) return depthNanos;
        if (!wantDepth) return colourNanos;
        return colourNanos - depthNanos >= 0 ? colourNanos : depthNanos;
    }
}

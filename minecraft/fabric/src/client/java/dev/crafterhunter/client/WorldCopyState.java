package dev.crafterhunter.client;

/** One capture's completion signals. Completion is not proof of valid pixels. */
public final class WorldCopyState {
    private final boolean wantColour;
    private final boolean wantDepth;
    private boolean colour;
    private boolean depth;

    public WorldCopyState(boolean wantColour, boolean wantDepth) {
        if (!wantColour && !wantDepth) throw new IllegalArgumentException("No attachment requested");
        this.wantColour = wantColour;
        this.wantDepth = wantDepth;
    }

    public synchronized void colourCompleted() { colour = true; }
    public synchronized void depthCompleted() { depth = true; }
    public synchronized boolean complete() {
        return (!wantColour || colour) && (!wantDepth || depth);
    }
}

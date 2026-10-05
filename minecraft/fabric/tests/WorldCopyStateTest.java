package dev.crafterhunter.client;

public final class WorldCopyStateTest {
    public static void main(String[] args) {
        WorldCopyState both = new WorldCopyState(true, true);
        check(!both.complete());
        both.colourCompleted();
        check(!both.complete());
        both.depthCompleted();
        check(both.complete());
        WorldCopyState colour = new WorldCopyState(true, false);
        colour.colourCompleted();
        check(colour.complete());
        WorldCopyState depth = new WorldCopyState(false, true);
        depth.depthCompleted();
        check(depth.complete());
        WorldCopyState next = new WorldCopyState(true, true);
        both.colourCompleted(); // late signal on old capture cannot complete next
        check(!next.complete());
        System.out.println("World copy completion checks passed");
    }
    private static void check(boolean value) {
        if (!value) throw new AssertionError("Incorrect completion state");
    }
}

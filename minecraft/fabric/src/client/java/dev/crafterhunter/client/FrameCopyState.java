package dev.crafterhunter.client;

/**
 * The capture's two-flag handshake, checked outside the game.
 *
 * The bug this exists to prevent: one flag standing for both "the copy is in
 * flight" and "the copy has landed". Publication then waits for a copy still
 * being written, and the request is re-queued forever — accepted, silent, and
 * producing nothing. That shipped and was only caught by reading a log, which
 * is exactly why the states are separated here and testable.
 */
public final class FrameCopyState {
    /** A copy has been queued and the callback has not fired. */
    public static final int IN_FLIGHT = 0;

    /** The callback fired: the buffer is readable on a later frame. */
    public static final int READY = 1;

    /** Nothing outstanding. */
    public static final int IDLE = 2;

    private FrameCopyState() {
    }

    /**
     * Advance the state machine one frame.
     *
     * @param current one of the three constants
     * @param copyCompletedNow whether the copy callback fired before this frame
     * @return the state for the next frame
     */
    public static int afterFrame(int current, boolean copyCompletedNow) {
        return switch (current) {
            case IN_FLIGHT -> copyCompletedNow ? READY : IN_FLIGHT;
            case READY -> IDLE;
            default -> IDLE;
        };
    }

    /** Whether a new copy may be queued this frame. */
    public static boolean mayQueue(int state) {
        return state == IDLE;
    }

    /** Whether this frame must publish what landed. */
    public static boolean mustPublish(int state) {
        return state == READY;
    }

    public static void check() {
        // A queued copy that never completes stays in flight and never
        // publishes, and never re-queues: it waits rather than spinning.
        int state = IN_FLIGHT;
        state = afterFrame(state, false);
        if (state != IN_FLIGHT) {
            throw new AssertionError("an unfinished copy must stay in flight");
        }
        if (mayQueue(state)) {
            throw new AssertionError("an unfinished copy must not be re-queued");
        }
        if (mustPublish(state)) {
            throw new AssertionError("an unfinished copy must not publish");
        }

        // The completion callback flips it to ready exactly once, and that is
        // the only transition that publishes.
        state = afterFrame(state, true);
        if (state != READY || mustPublish(state) != true) {
            throw new AssertionError("a completed copy must publish on the next frame");
        }
        if (mayQueue(state)) {
            throw new AssertionError("a ready copy must publish before another is queued");
        }

        // After publishing it returns to idle and can queue again.
        state = afterFrame(state, false);
        if (state != IDLE || !mayQueue(state)) {
            throw new AssertionError("a published copy must return to idle");
        }

        // A second callback for the same copy must not publish twice.
        if (mustPublish(afterFrame(state, true)) || mustPublish(afterFrame(READY, true))) {
            throw new AssertionError("one completion must publish exactly once");
        }

        // Idle with no request in flight is the state the game sits in.
        if (mustPublish(IDLE) || !mayQueue(IDLE)) {
            throw new AssertionError("idle must be publishable-free and queueable");
        }
    }
}

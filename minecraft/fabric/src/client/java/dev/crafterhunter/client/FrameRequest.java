package dev.crafterhunter.client;

import java.io.File;

/**
 * Reads one request line and nothing else, so the file contract can be checked
 * without a game. The real reader lives in the client sources, which need the
 * game's GPU classes to compile.
 */
public final class FrameRequest {
    public static final int DEFAULT_FRAMES = 1;
    public static final int MAX_FRAMES = 600;

    private FrameRequest() {
    }

    /**
     * Parse a request body: {@code capture [frames]}.
     *
     * Anything else is refused with the reason, because a request that is
     * quietly ignored looks exactly like a capture that quietly failed.
     */
    public static int frames(String body) throws IllegalArgumentException {
        if (body == null) {
            throw new IllegalArgumentException("empty request");
        }
        String[] parts = body.trim().split("\\s+");
        if (parts.length == 0 || parts[0].isEmpty()) {
            throw new IllegalArgumentException("empty request");
        }
        if (!parts[0].equals("capture")) {
            throw new IllegalArgumentException("unknown action: " + parts[0]);
        }
        int frames = DEFAULT_FRAMES;
        if (parts.length > 1) {
            try {
                frames = Integer.parseInt(parts[1]);
            } catch (NumberFormatException exception) {
                throw new IllegalArgumentException("frame count unreadable: " + parts[1]);
            }
        }
        if (frames < 1 || frames > MAX_FRAMES) {
            throw new IllegalArgumentException("frame count out of range: " + frames);
        }
        return frames;
    }

    /** The default request body, so a caller can write a valid one. */
    public static String defaultBody(int frames) {
        return "capture " + frames + "\n";
    }

    /** The default location, for tools that need to agree with the mod. */
    public static String defaultPath(File gameDirectory) {
        return new File(new File(gameDirectory, "crafterhunter"), "frame.request").getPath();
    }
}

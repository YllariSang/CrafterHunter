package dev.crafterhunter.client;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * The guest and the native reader, exercised together across the channel's lifecycle.
 *
 * <p>This is the test that was missing, and its absence is why a whole class of fault
 * survived: the guest's channel handling and the native reader had each been tested
 * alone. A predicate that correctly reports "the channel file is gone" can coexist with a
 * reader still serving a frame from an inode nothing writes to, and a guest can report a
 * perfectly healthy channel while the reader sees nothing. Neither statement is wrong and
 * together they are a bug neither test can see.
 *
 * <p>Every stage asserts on what the <em>probe</em> - the real reader, built from the real
 * headers - reports, at a path the guest really writes to. The guest-side predicate is
 * checked beside it rather than instead of it.
 *
 * <p>Channels are deliberately never closed: {@code FrameChannel.close()} deletes the
 * file, and the situation under test is a guest holding its channel open for the life of a
 * capture while the path underneath it changes. The handles live until the process exits.
 *
 * <p>Requires the probe path as the first argument.
 */
public final class FrameChannelRecoveryTest {

    /** The file both halves use, so no stage has to say which one it means. */
    private static Path CHANNEL;

    /** The sequence the reader last reported, carried across stages. */
    private static long lastSeenSequence = 0;

    /** Kept open for the life of the test, which is the point. */
    private static final List<FrameChannel> HELD = new ArrayList<>();

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            throw new IllegalArgumentException("usage: FrameChannelRecoveryTest <probe>");
        }
        Path probe = Path.of(args[0]);
        if (!Files.isExecutable(probe)) {
            throw new IllegalStateException("probe is not executable: " + probe);
        }

        Path directory = Files.createTempDirectory("crafterhunter-recovery");
        CHANNEL = directory.resolve("frame.channel");
        int width = 64;
        int height = 48;
        try {
            aPublishedFrameIsReadBack(probe, width, height);
            anUnlinkedChannelIsInvisibleToTheReader(probe, width, height);
            theReaderSeesTheRecreatedChannel(probe, width, height);
            theReaderSeesAResizedChannel(probe, width, height, 96, 64);
            theReaderSeesARestartedGuest(probe, width, height);
        } finally {
            for (FrameChannel channel : HELD) {
                try {
                    channel.close();
                } catch (IOException ignored) {
                    // Already gone, or going anyway: the assertions have already run.
                }
            }
            Files.deleteIfExists(CHANNEL);
            Files.deleteIfExists(directory);
        }
        System.out.println(
            "Frame recovery checks passed: frame read back, unlinked channel invisible to the"
                + " reader, recreated channel seen with a lower sequence, resized channel seen at"
                + " the new size, restarted guest seen.");
    }

    /** A channel left open at the path, with {@code frames} frames published into it. */
    private static FrameChannel held(int width, int height, int frames, byte fill)
            throws IOException {
        FrameChannel channel = new FrameChannel(CHANNEL, width, height);
        byte[] pixels = new byte[(int) FrameLayout.frameBytes(width, height)];
        Arrays.fill(pixels, fill);
        for (int i = 0; i < frames; i++) {
            channel.publish(pixels, 1_000L + i);
        }
        HELD.add(channel);
        return channel;
    }

    private static void aPublishedFrameIsReadBack(Path probe, int width, int height)
            throws Exception {
        held(width, height, 5, (byte) 0x44);
        Map<String, String> seen = readProbe(probe);
        long sequence = Long.parseLong(seen.get("SEQ"));
        if (sequence != 5) {
            throw new AssertionError("the reader must see the newest of five frames, saw " + sequence
                + " " + seen);
        }
        lastSeenSequence = sequence;
        if (!"1".equals(seen.get("VALID"))) {
            throw new AssertionError("and the header must validate: " + seen);
        }
        if (!String.valueOf(width).equals(seen.get("W"))
                || !String.valueOf(height).equals(seen.get("H"))) {
            throw new AssertionError("at the geometry the guest wrote: " + seen);
        }
    }

    /** The stage that matters most, because it is what actually happened. */
    private static void anUnlinkedChannelIsInvisibleToTheReader(Path probe, int width,
            int height) throws Exception {
        Files.delete(CHANNEL);
        if (Files.exists(CHANNEL)) {
            throw new AssertionError("the channel should be gone after deletion");
        }
        // The guest-side predicate must notice. This is the check Fabric previously did not
        // make, and not making it is why the guest carried on writing to an unlinked inode
        // and reported itself healthy.
        if (!FrameChannel.channelLost(CHANNEL, width, height)) {
            throw new AssertionError(
                "channelLost() must report a deleted channel as lost, or the guest writes to an"
                    + " unlinked inode and reports itself healthy while no reader can see it");
        }

        // The held channel is still open and still writable. Publishing into it must not
        // make the frame appear: this is the whole failure mode.
        byte[] pixels = new byte[(int) FrameLayout.frameBytes(width, height)];
        Arrays.fill(pixels, (byte) 0x55);
        HELD.get(0).publish(pixels, 9_000L);

        Map<String, String> during = readProbe(probe);
        if (Long.parseLong(during.get("SEQ")) != 0) {
            throw new AssertionError(
                "a reader must find no frame in a channel no longer at the path: " + during);
        }
        if (during.get("REASON").isEmpty()) {
            throw new AssertionError("and it must say why: " + during);
        }
    }

    /**
     * After the guest notices and reopens, the reader must see the new file - with
     * sequence numbers that restart, which is how it learns this is a new writer rather
     * than the one that went away.
     */
    private static void theReaderSeesTheRecreatedChannel(Path probe, int width, int height)
            throws Exception {
        FrameChannel replacement = held(width, height, 2, (byte) 0x66);
        if (FrameChannel.channelLost(CHANNEL, width, height)) {
            throw new AssertionError("a freshly written channel must not read as lost");
        }
        Map<String, String> after = readProbe(probe);
        long sequence = Long.parseLong(after.get("SEQ"));
        if (sequence != 2) {
            throw new AssertionError("the reader must see the recreated channel's newest frame,"
                + " saw " + sequence + " " + after);
        }
        if (sequence >= lastSeenSequence) {
            throw new AssertionError(
                "a recreated channel's sequence must be lower than the one the reader last saw,"
                    + " so it can tell this is a new writer; before=" + lastSeenSequence
                    + " after=" + sequence + " " + after);
        }
        lastSeenSequence = sequence;
        if (!"1".equals(after.get("VALID"))) {
            throw new AssertionError("and the header must validate: " + after);
        }
        HELD.remove(replacement);
    }

    /**
     * A resized guest writes a differently-sized buffer at the same path. The reader must
     * see the new geometry, and a mapping of exactly the new size - which is what the
     * page-rounded length would have got wrong.
     */
    private static void theReaderSeesAResizedChannel(Path probe, int width, int height,
            int newWidth, int newHeight) throws Exception {
        held(newWidth, newHeight, 3, (byte) 0x77);
        Map<String, String> resized = readProbe(probe);
        long sequence = Long.parseLong(resized.get("SEQ"));
        if (sequence != 3) {
            throw new AssertionError("the reader must see the resized channel's newest frame,"
                + " saw " + sequence + " " + resized);
        }
        lastSeenSequence = sequence;
        if (!String.valueOf(newWidth).equals(resized.get("W"))
                || !String.valueOf(newHeight).equals(resized.get("H"))) {
            throw new AssertionError("at the new geometry, not the old: " + resized);
        }
        long expected = FrameChannel.bufferBytes(newWidth, newHeight);
        long actual = Long.parseLong(resized.get("BYTES"));
        if (actual != expected) {
            throw new AssertionError("the mapping must be exactly the new buffer size " + expected
                + " bytes; got " + actual + ". A page-rounded length would be larger, and"
                + " would let a frame pass a bounds check it should fail.");
        }
    }

    /** A guest that restarts at the same geometry: its sequence numbers begin again. */
    private static void theReaderSeesARestartedGuest(Path probe, int width, int height)
            throws Exception {
        FrameChannel restarted = held(width, height, 1, (byte) 0x88);
        Map<String, String> seen = readProbe(probe);
        long sequence = Long.parseLong(seen.get("SEQ"));
        if (sequence != 1) {
            throw new AssertionError("a restarted guest begins its sequence at one, and the reader"
                + " must see that: " + seen);
        }
        if (sequence >= lastSeenSequence) {
            throw new AssertionError("and it must be lower than the sequence before the restart,"
                + " so the reader can tell a new writer from a stuck one; before="
                + lastSeenSequence + " after=" + sequence);
        }
        if (!String.valueOf(width).equals(seen.get("W"))
                || !String.valueOf(height).equals(seen.get("H"))) {
            throw new AssertionError("at the same geometry: " + seen);
        }
    }

    /** Runs the native reader against the channel path and parses its one line of output. */
    private static Map<String, String> readProbe(Path probe) throws Exception {
        List<String> command = new ArrayList<>();
        command.add(probe.toAbsolutePath().toString());
        command.add("--path");
        command.add(CHANNEL.toAbsolutePath().toString());
        ProcessBuilder builder = new ProcessBuilder(command);
        builder.redirectErrorStream(true);
        Process process = builder.start();
        StringBuilder output = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append('\n');
            }
        }
        process.waitFor();
        Map<String, String> fields = new HashMap<>();
        for (String token : output.toString().trim().split("\\s+")) {
            int equals = token.indexOf('=');
            if (equals > 0) {
                fields.put(token.substring(0, equals), token.substring(equals + 1));
            }
        }
        if (!fields.containsKey("SEQ")) {
            throw new AssertionError("probe produced no SEQ field: " + output);
        }
        if (!fields.containsKey("REASON")) {
            throw new AssertionError("probe produced no REASON field: " + output);
        }
        return fields;
    }
}

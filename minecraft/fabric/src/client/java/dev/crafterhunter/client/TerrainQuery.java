package dev.crafterhunter.client;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.*;
import java.util.UUID;

/** Explicit diagnostic segments, in host metres. Owned only by the UDP worker.
 * No physics, world edits, cached surfaces or render-thread work. */
final class TerrainQuery {
    private static final long TIMEOUT = 1_000_000_000L;
    private final Path directory;
    private long nonce, sentAt, nextPoll;
    private int wireId = new java.security.SecureRandom().nextInt();
    private boolean pending;

    TerrainQuery(Path directory) { this.directory = directory; }

    byte[] poll(long now, long wallMillis) throws IOException {
        if (pending) {
            if (now - sentAt >= TIMEOUT || now < sentAt) finish("timeout");
            return null;
        }
        if (now < nextPoll) return null;
        nextPoll = now + 50_000_000L;
        Path request = directory.resolve("terrain.request");
        Path claim = directory.resolve("terrain.claim-" + UUID.randomUUID());
        try { Files.move(request, claim, StandardCopyOption.ATOMIC_MOVE); }
        catch (NoSuchFileException absent) { return null; }
        try {
            nonce = 0;
            if (Files.size(claim) > 512) throw new IllegalArgumentException("request too large");
            String[] fields = Files.readString(claim).trim().split("\\s+");
            if (fields.length != 8) throw new IllegalArgumentException("request field count");
            nonce = Long.parseLong(fields[0]);
            long deadline = Long.parseLong(fields[1]);
            if (nonce <= 0 || deadline < wallMillis || deadline - wallMillis > 5000)
                throw new IllegalArgumentException("expired or invalid request");
            float[] segment = new float[6];
            for (int i = 0; i < 6; i++) segment[i] = Float.parseFloat(fields[i + 2]);
            byte[] payload = encode(++wireId, segment);
            sentAt = now;
            pending = true;
            return payload;
        } catch (IllegalArgumentException invalid) {
            finish("invalid");
            return null;
        } finally { Files.deleteIfExists(claim); }
    }

    static byte[] encode(int id, float[] segment) {
        if (segment.length != 6) throw new IllegalArgumentException("segment size");
        double length2 = 0;
        for (int i = 0; i < 6; i++) {
            if (!Float.isFinite(segment[i]) || Math.abs(segment[i]) > 100000)
                throw new IllegalArgumentException("segment range");
            if (i < 3) length2 += Math.pow((double)segment[i + 3] - segment[i], 2);
        }
        if (!(length2 > 0 && length2 <= 4096)) throw new IllegalArgumentException("segment length");
        ByteBuffer b = ByteBuffer.allocate(28).order(ByteOrder.LITTLE_ENDIAN).putInt(id);
        for (float v : segment) b.putFloat(v);
        return b.array();
    }

    void receive(byte[] payload, long now) throws IOException {
        if (!pending) return;
        if (now < sentAt || now - sentAt >= TIMEOUT) { finish("timeout"); return; }
        if (payload.length != 36) return;
        ByteBuffer b = ByteBuffer.wrap(payload).order(ByteOrder.LITTLE_ENDIAN);
        if (b.getInt() != wireId) return;
        int status = Byte.toUnsignedInt(b.get());
        if (status > 2 || b.get() != 0 || b.get() != 0 || b.get() != 0) return;
        float[] values = new float[6];
        for (int i = 0; i < 6; i++) {
            values[i] = b.getFloat();
            if (!Float.isFinite(values[i]) || (status != 2 && values[i] != 0)) return;
        }
        long attribute = Integer.toUnsignedLong(b.getInt());
        if (status != 2 && attribute != 0) return;
        String result = switch (status) { case 0 -> "unavailable"; case 1 -> "miss"; default -> "hit"; };
        if (status == 2) {
            for (float v : values) result += " " + Float.toString(v);
            result += " " + attribute;
        }
        finish(result);
    }

    void disconnect() throws IOException { if (pending) finish("disconnected"); }

    private void finish(String result) throws IOException {
        pending = false;
        Path temporary = Files.createTempFile(directory, "terrain-reply-", ".tmp");
        try {
            Files.writeString(temporary, nonce + " " + result + "\n");
            Files.move(temporary, directory.resolve("terrain.reply"),
                StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
        } finally { Files.deleteIfExists(temporary); }
    }
}

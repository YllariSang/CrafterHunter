package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** Small independent pose channel; never contains pixels or cross-process clock ages. */
public final class GuestCameraChannel {
    private GuestCameraChannel() { }
    public static byte[] encode(long generation, long request, long sequence, int mode,
            double[] position, float[] view, float[] projection) {
        if (generation <= 0 || request <= 0 || sequence <= 0 || mode < 0 || mode > 2
                || position.length != 3 || view.length != 16 || projection.length != 16)
            throw new IllegalArgumentException("invalid camera identity");
        ByteBuffer b = ByteBuffer.allocate(192).order(ByteOrder.LITTLE_ENDIAN);
        b.putInt(0x43474843).putInt(1).putLong(generation).putLong(request).putLong(sequence);
        b.putInt(mode).putInt(1);
        for (double v : position) {
            if (!Double.isFinite(v)) throw new IllegalArgumentException("nonfinite position");
            b.putDouble(v);
        }
        for (float[] matrix : new float[][] {view, projection}) for (float v : matrix) {
            if (!Float.isFinite(v)) throw new IllegalArgumentException("nonfinite camera matrix");
            b.putFloat(v);
        }
        return b.array();
    }

    /** An unchanged acknowledgement cannot hold ACTIVE indefinitely. */
    public static final class Acknowledgement {
        private long request, generation, epoch, sequence, advancedAt;
        private boolean active;
        public void observe(String text, long expectedRequest, long expectedGeneration, long now) {
            try {
                String[] fields = text.strip().split("\\s+");
                if (fields.length != 5) return;
                long r = Long.parseLong(fields[1]), g = Long.parseLong(fields[2]);
                long e = Long.parseLong(fields[3]), s = Long.parseLong(fields[4]);
                if (r != expectedRequest || g != expectedGeneration || e <= 0 || s <= 0) return;
                if (!fields[0].equals("active")) { active = false; return; }
                if (r != request || g != generation || e != epoch || s > sequence) {
                    request = r; generation = g; epoch = e; sequence = s; advancedAt = now;
                    active = true;
                }
            } catch (NumberFormatException exception) { /* malformed is never live */ }
        }
        public boolean active(long expectedRequest, long expectedGeneration, long now) {
            return active && request == expectedRequest && generation == expectedGeneration
                && now >= advancedAt && now - advancedAt < 500_000_000L;
        }
    }
}

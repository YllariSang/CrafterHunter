package dev.crafterhunter.client;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;

/** Request-paced paired snapshot, separate from the live colour channel. */
public final class WorldFrameChannel {
    public static final int MAGIC = 0x50574843; // CHWP in little endian
    public static final int VERSION = 1;
    public static final int HEADER_BYTES = 256;
    public static final int MAX_DIMENSION = 4096;
    private WorldFrameChannel() { }

    public static void publish(Path path, int width, int height, long generation,
            long identity, long issueNanos, float near, float far, float[] matrix,
            float[] pose, byte[] colour, byte[] depth) throws IOException {
        if (width < 1 || height < 1 || width > MAX_DIMENSION || height > MAX_DIMENSION
                || generation <= 0 || identity <= 0 || issueNanos <= 0
                || !Float.isFinite(near) || !Float.isFinite(far) || near <= 0 || far <= near
                || matrix.length != 16 || pose.length != 5) {
            throw new IllegalArgumentException("invalid world frame metadata");
        }
        for (float value : matrix) if (!Float.isFinite(value)) throw new IllegalArgumentException("matrix");
        for (float value : pose) if (!Float.isFinite(value)) throw new IllegalArgumentException("pose");
        int attachmentBytes = width * height * 4;
        if (colour.length != attachmentBytes || depth.length != attachmentBytes) {
            throw new IllegalArgumentException("attachments must match dimensions");
        }
        ByteBuffer header = ByteBuffer.allocate(HEADER_BYTES).order(ByteOrder.LITTLE_ENDIAN);
        // Flags=1 means bottom-up attachments. Depth is raw GL window depth;
        // projection defines its convention, never an assumed linear distance.
        header.putInt(MAGIC).putInt(VERSION).putInt(HEADER_BYTES).putInt(1);
        header.putInt(width).putInt(height).putInt(width * 4).putInt(0);
        header.putLong(generation).putLong(identity).putLong(issueNanos);
        header.putLong(attachmentBytes).putLong(attachmentBytes);
        header.putFloat(near).putFloat(far);
        for (float value : matrix) header.putFloat(value);
        for (float value : pose) header.putFloat(value);
        header.position(0);
        Files.createDirectories(path.toAbsolutePath().getParent());
        Path temporary = Files.createTempFile(path.toAbsolutePath().getParent(), "world-frame-", ".pending");
        try {
            try (FileChannel file = FileChannel.open(temporary, StandardOpenOption.WRITE)) {
                write(file, header);
                write(file, ByteBuffer.wrap(colour));
                write(file, ByteBuffer.wrap(depth));
            }
            Files.move(temporary, path, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
        } finally {
            Files.deleteIfExists(temporary); // only this publication's private temporary
        }
    }

    private static void write(FileChannel file, ByteBuffer bytes) throws IOException {
        while (bytes.hasRemaining()) {
            if (file.write(bytes) <= 0) throw new IOException("world frame write made no progress");
        }
    }
}

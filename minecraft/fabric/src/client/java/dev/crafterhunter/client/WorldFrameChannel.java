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
    public static final int VERSION = 5;
    public static final int HEADER_BYTES = 320;
    public static final int MAX_DIMENSION = 4096;
    private WorldFrameChannel() { }

    public static void publish(Path path, int width, int height, long generation,
            long identity, long issueNanos, float near, float far, float[] matrix,
            float[] pose, float[] view, double[] position, int clipMapping, int clipOrigin,
            int cameraMode, float rangeMin, float rangeMax, byte[] colour, byte[] depth) throws IOException {
        publish(path,width,height,generation,identity,issueNanos,near,far,matrix,pose,view,position,
            clipMapping,clipOrigin,cameraMode,rangeMin,rangeMax,null,colour,depth);
    }

    public static void publish(Path path, int width, int height, long generation,
            long identity, long issueNanos, float near, float far, float[] matrix,
            float[] pose, float[] view, double[] position, int clipMapping, int clipOrigin,
            int cameraMode, float rangeMin, float rangeMax, double[] playerPosition,
            byte[] colour, byte[] depth) throws IOException {
        publish(path,width,height,generation,identity,issueNanos,near,far,matrix,pose,view,position,
            clipMapping,clipOrigin,cameraMode,rangeMin,rangeMax,playerPosition,false,colour,depth);
    }

    public static void publish(Path path, int width, int height, long generation,
            long identity, long issueNanos, float near, float far, float[] matrix,
            float[] pose, float[] view, double[] position, int clipMapping, int clipOrigin,
            int cameraMode, float rangeMin, float rangeMax, double[] playerPosition,
            boolean bodyExcluded, byte[] colour, byte[] depth) throws IOException {
        // Archived callers preserve v4; live capture uses the issue-pose overload.
        publishFrame(path,width,height,generation,identity,issueNanos,near,far,matrix,pose,view,
            position,clipMapping,clipOrigin,cameraMode,rangeMin,rangeMax,playerPosition,
            bodyExcluded,null,colour,depth,4);
    }

    public static void publish(Path path, int width, int height, long generation,
            long identity, long issueNanos, float near, float far, float[] matrix,
            float[] pose, float[] view, double[] position, int clipMapping, int clipOrigin,
            int cameraMode, float rangeMin, float rangeMax, double[] playerPosition,
            boolean bodyExcluded, byte[] bodyPose, byte[] colour, byte[] depth) throws IOException {
        publishFrame(path,width,height,generation,identity,issueNanos,near,far,matrix,pose,view,
            position,clipMapping,clipOrigin,cameraMode,rangeMin,rangeMax,playerPosition,
            bodyExcluded,bodyPose,colour,depth,VERSION);
    }

    private static void publishFrame(Path path, int width, int height, long generation,
            long identity, long issueNanos, float near, float far, float[] matrix,
            float[] pose, float[] view, double[] position, int clipMapping, int clipOrigin,
            int cameraMode, float rangeMin, float rangeMax, double[] playerPosition,
            boolean bodyExcluded, byte[] bodyPose, byte[] colour, byte[] depth, int version) throws IOException {
        if (bodyExcluded && playerPosition == null) throw new IllegalArgumentException("body exclusion needs player provenance");
        if (version == VERSION && (bodyExcluded != (bodyPose != null)
                || bodyPose != null && !validBodyPose(bodyPose,generation)))
            throw new IllegalArgumentException("capture needs its exact valid body pose");
        if(playerPosition!=null) {
            if(playerPosition.length!=3) throw new IllegalArgumentException("player position");
            for(double value:playerPosition)
                if(!Double.isFinite(value)) throw new IllegalArgumentException("player position");
        }
        if (width < 1 || height < 1 || width > MAX_DIMENSION || height > MAX_DIMENSION
                || generation <= 0 || identity <= 0 || issueNanos <= 0
                || !Float.isFinite(near) || !Float.isFinite(far) || near <= 0 || far <= near
                || matrix.length != 16 || pose.length != 5 || view.length!=16 || position.length!=3
                || clipMapping<1 || clipMapping>2 || clipOrigin<1 || clipOrigin>2
                || cameraMode<0 || cameraMode>2 || rangeMin!=0 || rangeMax!=1) {
            throw new IllegalArgumentException("invalid world frame metadata");
        }
        for (float value : matrix) if (!Float.isFinite(value)) throw new IllegalArgumentException("matrix");
        for (float value : pose) if (!Float.isFinite(value)) throw new IllegalArgumentException("pose");
        for (float value : view) if (!Float.isFinite(value)) throw new IllegalArgumentException("view");
        for (double value : position) if (!Double.isFinite(value)) throw new IllegalArgumentException("position");
        int attachmentBytes = width * height * 4;
        if (colour.length != attachmentBytes || depth.length != attachmentBytes) {
            throw new IllegalArgumentException("attachments must match dimensions");
        }
        ByteBuffer header = ByteBuffer.allocate(HEADER_BYTES).order(ByteOrder.LITTLE_ENDIAN);
        // Flags bit 0 means bottom-up, v4 bit 1 means local body excluded.
        // Depth is raw GL window depth;
        // GL clip-control state is explicit; projection alone cannot define it.
        header.putInt(MAGIC).putInt(playerPosition==null ? 2 : version).putInt(HEADER_BYTES).putInt(bodyExcluded ? 3 : 1);
        header.putInt(width).putInt(height).putInt(width * 4).putInt(0);
        header.putLong(generation).putLong(identity).putLong(issueNanos);
        header.putLong(attachmentBytes).putLong(attachmentBytes);
        header.putFloat(near).putFloat(far);
        for (float value : matrix) header.putFloat(value);
        for (float value : pose) header.putFloat(value);
        header.putInt(clipMapping).putInt(clipOrigin).putFloat(0); // pinned main-pass clear
        for(float value:view) header.putFloat(value);
        for(double value:position) header.putDouble(value);
        header.putInt(cameraMode).putFloat(rangeMin).putFloat(rangeMax).putInt(1); // render-argument provenance
        if(playerPosition!=null) {
            for(double value:playerPosition) header.putDouble(value);
            header.putInt(1); // player feet sampled with this render frame's interpolation time
        }
        if (version == VERSION) header.putInt(308,bodyPose == null ? 0 : bodyPose.length);
        header.position(0);
        Files.createDirectories(path.toAbsolutePath().getParent());
        Path temporary = Files.createTempFile(path.toAbsolutePath().getParent(), "world-frame-", ".pending");
        try {
            try (FileChannel file = FileChannel.open(temporary, StandardOpenOption.WRITE)) {
                write(file, header);
                write(file, ByteBuffer.wrap(colour));
                write(file, ByteBuffer.wrap(depth));
                if (bodyPose != null) write(file, ByteBuffer.wrap(bodyPose));
            }
            Files.move(temporary, path, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
        } finally {
            Files.deleteIfExists(temporary); // only this publication's private temporary
        }
    }

    private static boolean validBodyPose(byte[] bytes,long generation) {
        if(bytes.length < 132 || bytes.length > 64+64*68) return false;
        ByteBuffer b=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        int parts=b.getInt(56);
        if(b.getInt(0)!=0x50504843 || b.getInt(4)!=1 || b.getLong(8)!=generation
                || b.getLong(16)<=0 || b.getLong(24)<=0 || b.getInt(60)!=0
                || parts<1 || parts>64 || bytes.length!=64+parts*68) return false;
        for(int i=0;i<3;i++) if(!Double.isFinite(b.getDouble(32+i*8))) return false;
        for(int i=0;i<parts;i++) {
            int at=64+i*68;
            if(b.getInt(at)!=0 && b.getInt(at)!=1) return false;
            for(int j=0;j<16;j++) if(!Float.isFinite(b.getFloat(at+4+j*4))) return false;
            if(b.getFloat(at+16)!=0 || b.getFloat(at+32)!=0 || b.getFloat(at+48)!=0
                    || b.getFloat(at+64)!=1) return false;
        }
        return true;
    }

    private static void write(FileChannel file, ByteBuffer bytes) throws IOException {
        while (bytes.hasRemaining()) {
            if (file.write(bytes) <= 0) throw new IOException("world frame write made no progress");
        }
    }
}

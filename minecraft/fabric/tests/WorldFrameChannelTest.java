package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicReference;

public final class WorldFrameChannelTest {
    private static void publish(Path file,int w,int h,long generation,long identity,long nanos,
            float near,float far,float[] matrix,float[] pose,byte[] colour,byte[] depth) throws Exception {
        WorldFrameChannel.publish(file,w,h,generation,identity,nanos,near,far,matrix,pose,
            new float[]{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
            new double[]{917.546812345,-58.38,343.1757},1,1,1,0,1,colour,depth);
    }
    public static void main(String[] args) throws Exception {
        Path file = Path.of(args[0]);
        float[] matrix = new float[16]; matrix[0]=1; matrix[5]=1; matrix[10]=0.00004883051f; matrix[11]=-1; matrix[14]=0.05000244f;
        float[] pose = {917.5f, -58.38f, 343.17f, 9.75f, 360.687f};
        byte[] colour = new byte[128], depth = new byte[128];
        Arrays.fill(colour, (byte)1); Arrays.fill(depth, (byte)1);
        publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,colour,depth);
        byte[] original = Files.readAllBytes(file);
        try {
            publish(file,8,4,99,2,1234,0.05f,1024,matrix,pose,new byte[1],depth);
            throw new AssertionError("wrong attachment accepted");
        } catch (IllegalArgumentException expected) { }
        if (!Arrays.equals(original, Files.readAllBytes(file))) throw new AssertionError("refusal replaced good frame");
        float[] view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        double[] position={917.546812345,-58.38,343.1757};
        for(int mode=0;mode<3;++mode) {
            WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,1,1,mode,0,1,colour,depth);
            Files.copy(file,file.resolveSibling("camera-"+mode+".frame"));
        }
        for(int refusal=0;refusal<3;++refusal) {
            try {
                WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                    refusal==0 ? 0:1,1,1,refusal==1 ? 0.2f:0,refusal==2 ? Float.NaN:1,colour,depth);
                throw new AssertionError("unknown convention/range accepted");
            } catch(IllegalArgumentException expected) { }
        }
        publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,colour,depth);
        AtomicReference<Throwable> failure = new AtomicReference<>();
        Thread reader = new Thread(() -> {
            try {
                for (int n=0; n<500; n++) {
                    byte[] bytes=Files.readAllBytes(file);
                    long identity=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getLong(40);
                    if (bytes.length != 576) throw new AssertionError("short file");
                    for (int i=320;i<bytes.length;i++) if (bytes[i] != (byte)identity) throw new AssertionError("torn pair");
                }
            } catch (Throwable problem) { failure.set(problem); }
        });
        reader.start();
        for (int identity=2; identity<=50; identity++) {
            Arrays.fill(colour,(byte)identity); Arrays.fill(depth,(byte)identity);
            publish(file,8,4,99,identity,1234,0.05f,1024,matrix,pose,colour,depth);
        }
        reader.join();
        if (failure.get()!=null) throw new AssertionError(failure.get());
        // Restart plus resize: no reader-visible truncation or mixed geometry.
        publish(file,4,4,100,1,5678,0.05f,1024,matrix,pose,new byte[64],new byte[64]);
        Files.copy(file,file.resolveSibling("resized.frame"));
        publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,originalSlice(original,320),originalSlice(original,448));
        System.out.println("PASS: paired publication, refusal, concurrent reads, restart and resize");
    }
    private static byte[] originalSlice(byte[] bytes, int start) { return Arrays.copyOfRange(bytes,start,start+128); }
}

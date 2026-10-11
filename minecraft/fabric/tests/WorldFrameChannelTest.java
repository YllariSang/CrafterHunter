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
            double[] feet={917.546812345,-60,343.1757};
            WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                1,1,mode,0,1,feet,colour,depth);
            Files.copy(file,file.resolveSibling("player-"+mode+".frame"));
            byte[] paired=Files.readAllBytes(file);
            if(ByteBuffer.wrap(paired).order(ByteOrder.LITTLE_ENDIAN).getInt(4)!=4)
                throw new AssertionError("player reference must be versioned");
            WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                1,1,mode,0,1,feet,true,colour,depth);
            Files.copy(file,file.resolveSibling("separate-"+mode+".frame"));
            if(ByteBuffer.wrap(Files.readAllBytes(file)).order(ByteOrder.LITTLE_ENDIAN).getInt(12)!=3)
                throw new AssertionError("missing body-exclusion provenance");
            float[] bone={1,0,0,0,0,1,0,0,0,0,1,0,3,0,0,1};
            byte[] issued=PlayerModelChannel.pose(99,42,17,feet[0],feet[1],feet[2],
                java.util.List.of(bone),java.util.List.of(true));
            // A later pose exists when the older GPU capture finishes.
            byte[] later=PlayerModelChannel.pose(99,42,18,feet[0]+5,feet[1],feet[2],
                java.util.List.of(bone),java.util.List.of(true));
            Files.write(file.resolveSibling("latest-player.pose"),later);
            WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                1,1,mode,0,1,feet,true,issued,colour,depth);
            Files.copy(file,file.resolveSibling("capture-"+mode+".frame"));
            byte[] complete=Files.readAllBytes(file);
            ByteBuffer header=ByteBuffer.wrap(complete).order(ByteOrder.LITTLE_ENDIAN);
            if(header.getInt(4)!=5 || header.getInt(308)!=issued.length
                    || !Arrays.equals(issued,Arrays.copyOfRange(complete,576,complete.length)))
                throw new AssertionError("capture lost its issue-time pose");
            for(int mutation=0;mutation<7;mutation++) {
                byte[] wrong=issued.clone();
                ByteBuffer bb=ByteBuffer.wrap(wrong).order(ByteOrder.LITTLE_ENDIAN);
                switch(mutation) {
                    case 0 -> bb.putLong(8,100);
                    case 1 -> bb.putLong(16,0);
                    case 2 -> bb.putLong(24,0);
                    case 3 -> bb.putInt(56,65);
                    case 4 -> bb.putDouble(32,Double.NaN);
                    case 5 -> bb.putFloat(80,1); // nonaffine W row
                    case 6 -> bb.putInt(64,2);
                }
                try {
                    WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                        1,1,mode,0,1,feet,true,wrong,colour,depth);
                    throw new AssertionError("bad issue-time pose accepted");
                } catch(IllegalArgumentException expected) { }
                if(!Arrays.equals(complete,Files.readAllBytes(file))) throw new AssertionError("bad pose replaced pair");
            }
            for(boolean excluded:new boolean[]{false,true}) {
                try {
                    WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                        1,1,mode,0,1,feet,excluded,excluded ? null:issued,colour,depth);
                    throw new AssertionError("exclusion/pose disagreement accepted");
                } catch(IllegalArgumentException expected) { }
            }
            WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                1,1,mode,0,1,feet,false,null,colour,depth);
            Files.copy(file,file.resolveSibling("unfiltered-"+mode+".frame"));
            Files.write(file,paired);
            feet[1]=Double.NaN;
            try {
                WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                    1,1,mode,0,1,feet,colour,depth);
                throw new AssertionError("nonfinite player accepted");
            } catch(IllegalArgumentException expected) { }
            if(!Arrays.equals(paired,Files.readAllBytes(file))) throw new AssertionError("bad player replaced pair");
        }
        try {
            WorldFrameChannel.publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,view,position,
                1,1,1,0,1,null,true,colour,depth);
            throw new AssertionError("excluded body without player provenance accepted");
        } catch(IllegalArgumentException expected) { }
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
        // Concurrent reads must bind pixels AND serialized body to one capture inode.
        double[] feet={0,0,0};
        float[] bone={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        Arrays.fill(colour,(byte)60);Arrays.fill(depth,(byte)60);
        byte[] body=PlayerModelChannel.pose(99,42,60,0,0,0,java.util.List.of(bone),java.util.List.of(true));
        WorldFrameChannel.publish(file,8,4,99,60,1234,0.05f,1024,matrix,pose,view,position,
            1,1,1,0,1,feet,true,body,colour,depth);
        Thread pairedReader=new Thread(() -> {
            try {
                for(int n=0;n<500;n++) {
                    byte[] bytes=Files.readAllBytes(file);
                    ByteBuffer b=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
                    long id=b.getLong(40);
                    if(bytes.length!=708 || b.getInt(4)!=5 || b.getLong(600)!=id)
                        throw new AssertionError("pose/pixels torn across captures");
                    for(int i=320;i<576;i++) if(bytes[i]!=(byte)id) throw new AssertionError("mixed attachments");
                }
            } catch(Throwable problem) { failure.set(problem); }
        });
        pairedReader.start();
        for(int id=61;id<=110;id++) {
            Arrays.fill(colour,(byte)id);Arrays.fill(depth,(byte)id);
            body=PlayerModelChannel.pose(99,42,id,0,0,0,java.util.List.of(bone),java.util.List.of(true));
            WorldFrameChannel.publish(file,8,4,99,id,1234,0.05f,1024,matrix,pose,view,position,
                1,1,1,0,1,feet,true,body,colour,depth);
        }
        pairedReader.join();
        if(failure.get()!=null) throw new AssertionError(failure.get());
        // Restart plus resize: no reader-visible truncation or mixed geometry.
        publish(file,4,4,100,1,5678,0.05f,1024,matrix,pose,new byte[64],new byte[64]);
        Files.copy(file,file.resolveSibling("resized.frame"));
        publish(file,8,4,99,1,1234,0.05f,1024,matrix,pose,originalSlice(original,320),originalSlice(original,448));
        System.out.println("PASS: paired publication, refusal, concurrent reads, restart and resize");
    }
    private static byte[] originalSlice(byte[] bytes, int start) { return Arrays.copyOfRange(bytes,start,start+128); }
}

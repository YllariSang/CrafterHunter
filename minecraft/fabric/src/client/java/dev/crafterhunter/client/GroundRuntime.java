package dev.crafterhunter.client;

import java.nio.file.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicLong;
import net.minecraft.client.Minecraft;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.*;

/** Client-tick I/O; immutable binding consumed by client and integrated-server movement.
 * No file I/O, game writes or client singleton access on the collision path. */
public final class GroundRuntime {
    private static final Path PATCH=Path.of("/dev/shm/crafterhunter/ground.patch");
    private static final Path ACK_PATH=Path.of("/dev/shm/crafterhunter/camera.reply");
    private static final AtomicLong CLIENT=new AtomicLong(),SERVER=new AtomicLong();
    private record Binding(UUID player, Level client, Level server, VoxelShape shape, long expires) { }
    private static volatile Binding binding;
    private static GroundPatchChannel channel=new GroundPatchChannel();
    private static GuestCameraChannel.Acknowledgement ack=new GuestCameraChannel.Acknowledgement();
    private static GroundPatchChannel.Frame frame;
    private static String priorText="";
    private static long priorToken,priorEpoch,reportAt,revokedToken;
    private static Object priorWorld;
    private GroundRuntime() { }

    public static void clear() {
        binding=null; frame=null; priorText=""; priorToken=0;priorEpoch=0;priorWorld=null;
        channel=new GroundPatchChannel();ack=new GuestCameraChannel.Acknowledgement();
    }
    public static void tick() {
        Minecraft mc=Minecraft.getInstance();
        long now=System.nanoTime(), token=GuestCameraControl.token();
        if(priorWorld!=null && priorWorld!=mc.level) revokedToken=token;
        if(token!=0 && token==revokedToken) { binding=null;frame=null;return; }
        if(token==0 || mc.player==null || mc.level==null || mc.isPaused()
                || mc.getSingleplayerServer()==null || !mc.player.getAbilities().mayfly) { clear(); return; }
        try {
            long generation=WorldCapture.instance().generation();
            if(Files.size(ACK_PATH)>160) { clear(); return; }
            String text=Files.readString(ACK_PATH);String[] fields=text.strip().split("\\s+");
            if(fields.length!=5 || !fields[0].equals("active")) { clear(); return; }
            long epoch=Long.parseLong(fields[3]);
            if(priorToken!=token || priorEpoch!=epoch || priorWorld!=mc.level) {
                clear();priorToken=token;priorEpoch=epoch;priorWorld=mc.level;
            }
            ack.observe(text,token,generation,now);
            if(!ack.active(token,generation,now)) { binding=null;frame=null;return; }
            if(Files.size(PATCH)>4096) { binding=null;frame=null;return; }
            String payload=Files.readString(PATCH);
            if(!payload.equals(priorText)) {
                priorText=payload;
                frame=channel.decode(payload,token,generation,epoch,now,System.currentTimeMillis());
            }
            if(frame==null || now>=frame.expiresNanos()) { binding=null;return; }
            var server=mc.getSingleplayerServer().getLevel(mc.level.dimension());
            if(server==null) { clear();return; }
            var p=frame.patch();
            VoxelShape shape=Shapes.empty();
            for(var cell:frame.patches()) shape=Shapes.or(shape,
                Shapes.box(cell.minX(),cell.top()-.05,cell.minZ(),cell.maxX(),cell.top(),cell.maxZ()));
            binding=new Binding(mc.player.getUUID(),mc.level,server,
                shape,
                Math.min(frame.expiresNanos(),Math.min(ack.expiresNanos(),now+100_000_000L)));
            if(now>=reportAt) {
                reportAt=now+1_000_000_000L;
                System.out.println("[CrafterHunter] ground patch seq="+frame.sequence()+" token="+token
                    +" epoch="+epoch+" cells="+frame.patches().size()+" top="+p.top()+" client-colliders="+CLIENT.get()+" server-colliders="+SERVER.get());
            }
        } catch(java.io.IOException | IllegalArgumentException e) { binding=null;frame=null; }
    }
    public static List<VoxelShape> append(Entity entity, Level level, AABB box, List<VoxelShape> original) {
        Binding b=binding;
        if(b==null || System.nanoTime()>=b.expires || !(entity instanceof Player player)
                || !entity.getUUID().equals(b.player) || (level!=b.client && level!=b.server)
                || entity.noPhysics || entity.isSpectator() || !player.getAbilities().mayfly || player.getAbilities().flying
                || !box.intersects(b.shape.bounds())) return original;
        ArrayList<VoxelShape> shapes=new ArrayList<>(original);shapes.add(b.shape);
        (level==b.client?CLIENT:SERVER).incrementAndGet();return shapes;
    }
    public static String hudLine() {
        Binding b=binding;
        return "GROUND: "+(b!=null && System.nanoTime()<b.expires ? "test patch" : "OFF");
    }
}

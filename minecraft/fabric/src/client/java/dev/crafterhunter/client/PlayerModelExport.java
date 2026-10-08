package dev.crafterhunter.client;

import com.mojang.blaze3d.opengl.GlTexture;
import com.mojang.blaze3d.vertex.PoseStack;
import dev.crafterhunter.client.mixin.ModelPartAccessor;
import java.lang.reflect.Proxy;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.TreeMap;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.Minecraft;
import net.minecraft.client.model.geom.ModelPart;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.client.renderer.entity.state.AvatarRenderState;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import org.lwjgl.opengl.GL11C;
import org.lwjgl.opengl.GL15C;
import org.lwjgl.opengl.GL21C;
import org.lwjgl.opengl.GL45C;
import org.lwjgl.system.MemoryUtil;

/** Opt-in CPU baked player mesh + resident skin; per-part pose, not framebuffer reconstruction. */
public final class PlayerModelExport {
    private static final Path CHANNEL = Path.of("/dev/shm/crafterhunter");
    private static final Path ENABLED = FabricLoader.getInstance().getGameDir().resolve("crafterhunter/player-export.enabled");
    private static long next, sequence, assetIdentity;
    private static byte[] lastGeometry, skin;
    private static GlTexture skinTexture;
    private static int skinWidth, skinHeight;
    private static int lastParts;
    private static boolean failed;
    private PlayerModelExport() {}

    public static void capture(float partialTick, CameraRenderState camera) {
        long now = System.nanoTime();
        if (failed || now < next) return;
        next = now + 250_000_000L; // Same bounded 4 Hz preview budget; not a 60 Hz claim.
        if (!Files.exists(ENABLED)) return;
        var mc = Minecraft.getInstance();
        if (mc.player == null || mc.level == null || mc.getCameraEntity() != mc.player) return;
        try {
            var renderer = mc.getEntityRenderDispatcher().getPlayerRenderer(mc.player);
            AvatarRenderState state = renderer.createRenderState();
            renderer.extractRenderState(mc.player, state, partialTick);
            if (state.isInvisible || state.isSpectator) return;
            var texture = mc.getTextureManager().getTexture(state.skin.body().texturePath()).getTexture();
            if (!(texture instanceof GlTexture gl) || gl.isClosed()) throw new IllegalStateException("player skin requires live OpenGL texture");
            if (gl != skinTexture) {
                skinWidth = gl.getWidth(0); skinHeight = gl.getHeight(0);
                if (skinWidth < 1 || skinHeight < 1 || skinWidth > 256 || skinHeight > 256)
                    throw new IllegalStateException("unsupported skin dimensions");
                skin = readSkin(gl, skinWidth * skinHeight * 4);
                skinTexture = gl;
                lastGeometry = null; // Resource reload/skin replacement must replace resident asset.
            }
            var model = renderer.getModel();
            var parts = model.allParts();
            var oldPoses = parts.stream().map(ModelPart::storePose).toList();
            boolean[] visible = new boolean[parts.size()], skip = new boolean[parts.size()];
            for (int i=0;i<parts.size();i++) { visible[i]=parts.get(i).visible; skip[i]=parts.get(i).skipDraw; }
            final boolean[] captured = {false};
            // Receiver only consumes the main player model. It never queues actual draws,
            // items, armor, names, capes or world entities. submit() supplies vanilla
            // body orientation/scale/sleep/flight transforms; setupAnim supplies bones.
            SubmitNodeCollector collector = (SubmitNodeCollector) Proxy.newProxyInstance(
                SubmitNodeCollector.class.getClassLoader(), new Class<?>[]{SubmitNodeCollector.class},
                (proxy, method, args) -> {
                    if (method.getName().equals("order")) return proxy;
                    if (method.getName().equals("submitModel") && args[0] == model && !captured[0]) {
                        model.setupAnim(state);
                        publish(model.root(), (PoseStack) args[2], state);
                        captured[0] = true;
                    }
                    return null;
                });
            try {
                // A feet-relative PoseStack avoids large-world float precision loss.
                PoseStack pose = new PoseStack();
                var offset = renderer.getRenderOffset(state);
                pose.translate(offset.x, offset.y, offset.z);
                renderer.submit(state, pose, collector, camera);
            } finally {
                for (int i=0;i<parts.size();i++) {
                    parts.get(i).loadPose(oldPoses.get(i));
                    parts.get(i).visible=visible[i]; parts.get(i).skipDraw=skip[i];
                }
            }
        } catch (Exception e) {
            failed = true; // No per-frame exception loops or uncertain texture reuse.
            System.err.println("CrafterHunter player export disabled until restart: " + e);
        }
    }

    private static void publish(ModelPart root, PoseStack stack, AvatarRenderState state) throws Exception {
        var vertices = new ArrayList<float[]>();
        var matrices = new ArrayList<float[]>();
        var visibility = new ArrayList<Boolean>();
        collect(root, stack, true, vertices, matrices, visibility);
        if (matrices.size()>64 || vertices.isEmpty() || vertices.size()>4096) throw new IllegalStateException("player mesh bound");
        ByteBuffer geometry = ByteBuffer.allocate(vertices.size()*24).order(ByteOrder.LITTLE_ENDIAN);
        for (float[] vertex : vertices) {
            for (int i=0;i<5;i++) geometry.putFloat(vertex[i]);
            geometry.putInt((int)vertex[5]);
        }
        byte[] bytes = geometry.array();
        long generation = WorldCapture.instance().generation();
        if (!Arrays.equals(bytes,lastGeometry) || lastParts!=matrices.size()) {
            long id = assetIdentity+1;
            atomic("player.asset",PlayerModelChannel.asset(generation,id,matrices.size(),skinWidth,skinHeight,bytes,skin));
            lastGeometry=bytes; lastParts=matrices.size(); assetIdentity=id;
            System.out.println("CrafterHunter player asset published: parts="+matrices.size()+" vertices="+vertices.size()+" skin="+skinWidth+"x"+skinHeight);
        }
        atomic("player.pose",PlayerModelChannel.pose(generation,assetIdentity,++sequence,
            state.x,state.y,state.z,matrices,visibility));
    }

    private static void collect(ModelPart part, PoseStack stack, boolean parentVisible,
            ArrayList<float[]> vertices, ArrayList<float[]> matrices, ArrayList<Boolean> visibility) {
        if (matrices.size()>=64) throw new IllegalStateException("part bound");
        int bone=matrices.size();
        stack.pushPose(); part.translateAndRotate(stack);
        matrices.add(stack.last().pose().get(new float[16]));
        boolean shown=parentVisible && part.visible;
        visibility.add(shown && !part.skipDraw);
        var access=(ModelPartAccessor)(Object)part;
        for (var cube:access.crafterhunter$cubes()) for (var polygon:cube.polygons) {
            var quad=polygon.vertices();
            if (quad.length!=4) throw new IllegalStateException("nonquad player polygon");
            for (int index:new int[]{0,1,2,0,2,3}) {
                var v=quad[index];
                vertices.add(new float[]{v.worldX(),v.worldY(),v.worldZ(),v.u(),v.v(),bone});
            }
        }
        for (var child:new TreeMap<>(access.crafterhunter$children()).values())
            collect(child,stack,shown,vertices,matrices,visibility);
        stack.popPose();
    }

    private static byte[] readSkin(GlTexture texture,int size) {
        // DSA readback of the tiny complete skin, not a camera image. Restore all
        // pack state affecting this read, and never interfere with MC's PBO copies.
        int[] names={GL11C.GL_PACK_ALIGNMENT,GL11C.GL_PACK_ROW_LENGTH,GL11C.GL_PACK_SKIP_PIXELS,
            GL11C.GL_PACK_SKIP_ROWS,GL11C.GL_PACK_SWAP_BYTES};
        int[] old=new int[names.length];
        for(int i=0;i<names.length;i++) old[i]=GL11C.glGetInteger(names[i]);
        int pbo=GL11C.glGetInteger(GL21C.GL_PIXEL_PACK_BUFFER_BINDING);
        ByteBuffer data=MemoryUtil.memAlloc(size);
        try {
            GL15C.glBindBuffer(GL21C.GL_PIXEL_PACK_BUFFER,0);
            for(int i=0;i<names.length;i++) GL11C.glPixelStorei(names[i],i==0?1:0);
            GL45C.glGetTextureImage(texture.glId(),0,GL11C.GL_RGBA,GL11C.GL_UNSIGNED_BYTE,data);
            int error=GL11C.glGetError();
            if(error!=GL11C.GL_NO_ERROR) throw new IllegalStateException("skin read GL error "+error);
            byte[] result=new byte[size]; data.get(result); return result;
        } finally {
            for(int i=0;i<names.length;i++) GL11C.glPixelStorei(names[i],old[i]);
            GL15C.glBindBuffer(GL21C.GL_PIXEL_PACK_BUFFER,pbo); MemoryUtil.memFree(data);
        }
    }

    private static void atomic(String name,byte[] bytes) throws Exception {
        Files.createDirectories(CHANNEL);
        Path temporary=Files.createTempFile(CHANNEL,"player-",".tmp");
        try { Files.write(temporary,bytes); Files.move(temporary,CHANNEL.resolve(name),
            StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING); }
        finally { Files.deleteIfExists(temporary); }
    }
}

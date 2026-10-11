package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.WorldCapture;
import dev.crafterhunter.client.BodyCapturePass;
import dev.crafterhunter.client.PlayerBodyCapture;
import com.mojang.blaze3d.framegraph.FrameGraphBuilder;
import net.minecraft.client.renderer.LevelRenderer;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import net.minecraft.client.DeltaTracker;
import com.mojang.blaze3d.resource.GraphicsResourceAllocator;
import com.mojang.blaze3d.buffers.GpuBufferSlice;
import org.joml.Matrix4fc;
import org.joml.Vector4f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** Capture scene attachments before the optional always-on-top pass erases depth. */
@Mixin(LevelRenderer.class)
public abstract class LevelWorldCaptureMixin {
    @Inject(method = "render", at = @At("HEAD"), require = 1)
    private void crafterhunter$recordView(GraphicsResourceAllocator allocator, DeltaTracker delta,
            boolean outline, CameraRenderState camera, Matrix4fc view, GpuBufferSlice fog,
            Vector4f colour, boolean worldFog, CallbackInfo callback) {
        WorldCapture.instance().recordWorldView(camera, view, delta.getGameTimeDeltaPartialTick(false));
        dev.crafterhunter.client.GuestCameraControl.capture(camera, view);
        dev.crafterhunter.client.PlayerModelExport.capture(delta.getGameTimeDeltaPartialTick(false), camera);
        PlayerBodyCapture.begin();
    }
    @Redirect(method = "render", require = 1, at = @At(value = "INVOKE",
        target = "Lcom/mojang/blaze3d/framegraph/FrameGraphBuilder;execute(Lcom/mojang/blaze3d/resource/GraphicsResourceAllocator;Lcom/mojang/blaze3d/framegraph/FrameGraphBuilder$Inspector;)V"))
    private void crafterhunter$bodyFreeCapture(FrameGraphBuilder graph, GraphicsResourceAllocator allocator,
            FrameGraphBuilder.Inspector inspector) {
        BodyCapturePass.execute(graph, allocator, inspector, PlayerBodyCapture.filteredPass(),
            () -> WorldCapture.instance().beforeWorldDepthClear("pre-hand-clear (no always-on-top pass)"));
    }
    // Version-pinned synthetic method: require a match, never silently skip it.
    @Inject(method = "lambda$addAlwaysOnTopPass$0", remap = false, require = 1,
        at = @At(value = "INVOKE",
            target = "Lcom/mojang/blaze3d/systems/CommandEncoder;clearDepthTexture(Lcom/mojang/blaze3d/textures/GpuTexture;D)V"))
    private void crafterhunter$beforeAlwaysOnTopClear(CallbackInfo callback) {
        WorldCapture.instance().beforeWorldDepthClear("pre-always-on-top-clear");
    }
}

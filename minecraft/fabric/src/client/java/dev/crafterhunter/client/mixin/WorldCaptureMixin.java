package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.WorldCapture;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyArg;
import org.joml.Matrix4f;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Captures in the one window where world colour and world depth are both complete:
 * inside the final always-on-top frame-graph pass, before it clears scene depth.
 *
 * <p>The previous hook sat at {@code renderLevel} TAIL - after the level and the hand,
 * before the GUI - which kept HUD pixels out, but also after the game's own
 * {@code clearDepthTexture(mainRenderTarget.getDepthTexture(), 0.0)} that runs between
 * the level pass and {@code renderItemInHand}. Minecraft 26.2's level renders through a
 * frame graph whose {@code main} is imported from the main render target, so world
 * depth is initially drawn to that main target, then destructive clears erase it. The
 * measured consequence (archive 20261006T172918Z): a depth buffer that was 95% exact
 * clear zeros, with only the held item and the post-clear centre marker surviving,
 * while colour held the full scene. Reading after an erase cannot be repaired
 * downstream. There is ALSO a clear inside LevelRenderer's always-on-top pass,
 * before renderLevel resumes. Capturing before the later hand clear is too late:
 * the latest live capture contained only zero depth. Issue before the earlier clear
 * instead, after the preceding world passes have executed.
 *
 * <p>What this hook now excludes is the first-person hand: {@code renderItemInHand}
 * runs <em>after</em> the clear, downstream of this point, so neither hand nor GUI is
 * in the capture. That is recorded rather than assumed - {@code handInFrame} in the
 * metadata states it per capture, and the colour path ({@code FrameCaptureMixin} at
 * {@code GameRenderer.render}) is deliberately untouched.
 */
@Mixin(GameRenderer.class)
public abstract class WorldCaptureMixin {
    @Inject(method = "renderLevel", at = @At("HEAD"), require = 1)
    private void crafterhunter$beginWorld(DeltaTracker delta, CallbackInfo callback) {
        WorldCapture.instance().beginWorldFrame();
    }

    @ModifyArg(method = "renderLevel", require = 1, index = 0,
        at = @At(value = "INVOKE", target = "Lnet/minecraft/client/renderer/ProjectionMatrixBuffer;getBuffer(Lorg/joml/Matrix4f;)Lcom/mojang/blaze3d/buffers/GpuBufferSlice;"))
    private Matrix4f crafterhunter$recordUploadedProjection(Matrix4f matrix) {
        WorldCapture.instance().recordWorldProjection(matrix);
        return matrix; // observe, never modify Minecraft's camera
    }

    // The always-on-top pass is conditional. When absent, the hand clear is
    // the first destructive clear; when present, the earlier hook wins.
    @Inject(method = "renderLevel", require = 1,
        at = @At(value = "INVOKE",
            target = "Lcom/mojang/blaze3d/systems/CommandEncoder;clearDepthTexture(Lcom/mojang/blaze3d/textures/GpuTexture;D)V"))
    private void crafterhunter$captureWorld(DeltaTracker delta, CallbackInfo callback) {
        WorldCapture.instance().beforeWorldDepthClear("pre-hand-clear (no always-on-top pass)");
    }
}

package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.WorldCapture;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Captures in the one window where world colour and world depth are both complete:
 * after {@code LevelRenderer.render} returns, before {@code GameRenderer.renderLevel}
 * clears the main target's depth.
 *
 * <p>The previous hook sat at {@code renderLevel} TAIL - after the level and the hand,
 * before the GUI - which kept HUD pixels out, but also after the game's own
 * {@code clearDepthTexture(mainRenderTarget.getDepthTexture(), 0.0)} that runs between
 * the level pass and {@code renderItemInHand}. Minecraft 26.2's level renders through a
 * frame graph whose {@code main} is imported from the main render target, so world
 * depth <em>is</em> in the main target once {@code FrameGraphBuilder.execute} has run
 * inside {@code LevelRenderer.render} - and the clear then erases it before TAIL. The
 * measured consequence (archive 20261006T172918Z): a depth buffer that was 95% exact
 * clear zeros, with only the held item and the post-clear centre marker surviving,
 * while colour held the full scene. Reading after an erase cannot be repaired
 * downstream; issuing here, immediately before the clear, is the only point where the
 * world's own depth still exists.
 *
 * <p>What this hook now excludes is the first-person hand: {@code renderItemInHand}
 * runs <em>after</em> the clear, downstream of this point, so neither hand nor GUI is
 * in the capture. That is recorded rather than assumed - {@code handInFrame} in the
 * metadata states it per capture, and the colour path ({@code FrameCaptureMixin} at
 * {@code GameRenderer.render}) is deliberately untouched.
 */
@Mixin(GameRenderer.class)
public abstract class WorldCaptureMixin {

    @Inject(method = "renderLevel", at = @At(value = "INVOKE",
        target = "Lcom/mojang/blaze3d/systems/CommandEncoder;clearDepthTexture(Lcom/mojang/blaze3d/textures/GpuTexture;D)V"))
    private void crafterhunter$captureWorld(DeltaTracker delta, CallbackInfo callback) {
        WorldCapture capture = WorldCapture.instance();
        capture.markHookSeen();
        capture.onWorldRendered(System.nanoTime());
    }
}

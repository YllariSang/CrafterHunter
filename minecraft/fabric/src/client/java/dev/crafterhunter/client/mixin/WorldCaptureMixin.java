package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.WorldCapture;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Captures at the end of the <em>world</em> render, not the end of the frame.
 *
 * <p>The colour path captures at {@code GameRenderer.render} TAIL, which is after GUI
 * compositing — and that is why our own HUD text and Minecraft's F3 debug screen both
 * appear in the frames handed to MHW. A world capture wants the other boundary:
 * {@code renderLevel} has finished drawing the level and the GUI has not yet been
 * composited, so the texture holds world pixels only.
 *
 * <p>What this hook does <em>not</em> exclude is the first-person hand.
 * {@code renderItemInHand} is called from inside the level pass, so no injection point
 * in this method is upstream of it. Rather than pretend otherwise, the capture records
 * the camera perspective it was taken at, so a frame captured in first person can be
 * recognised and excluded from any measurement that assumes the frame is world-only.
 */
@Mixin(GameRenderer.class)
public abstract class WorldCaptureMixin {

    @Inject(method = "renderLevel", at = @At("TAIL"))
    private void crafterhunter$captureWorld(DeltaTracker delta, CallbackInfo callback) {
        WorldCapture capture = WorldCapture.instance();
        capture.markHookSeen();
        capture.onWorldRendered(System.nanoTime());
    }
}

package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.FrameCapture;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Runs the frame reader at the end of the frame.
 *
 * The tail of {@code render} is chosen over the level renderer on purpose for
 * the first measurement: it includes the HUD, which is a cheap way to confirm
 * the readback is neither upside down nor from the wrong buffer before any
 * native work assumes otherwise.
 */
@Mixin(GameRenderer.class)
public abstract class FrameCaptureMixin {
    @Inject(method = "render", at = @At("TAIL"))
    private void crafterhunter$captureFrame(DeltaTracker delta, boolean renderLevel,
                                            CallbackInfo callback) {
        FrameCapture.instance().onRenderedFrame(System.nanoTime());
    }
}

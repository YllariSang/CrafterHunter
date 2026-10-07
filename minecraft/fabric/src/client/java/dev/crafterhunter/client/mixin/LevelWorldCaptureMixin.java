package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.WorldCapture;
import net.minecraft.client.renderer.LevelRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** Capture scene attachments before the optional always-on-top pass erases depth. */
@Mixin(LevelRenderer.class)
public abstract class LevelWorldCaptureMixin {
    // Version-pinned synthetic method: require a match, never silently skip it.
    @Inject(method = "lambda$addAlwaysOnTopPass$0", remap = false, require = 1,
        at = @At(value = "INVOKE",
            target = "Lcom/mojang/blaze3d/systems/CommandEncoder;clearDepthTexture(Lcom/mojang/blaze3d/textures/GpuTexture;D)V"))
    private void crafterhunter$beforeAlwaysOnTopClear(CallbackInfo callback) {
        WorldCapture.instance().beforeWorldDepthClear("pre-always-on-top-clear");
    }
}

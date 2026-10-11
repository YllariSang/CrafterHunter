package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.PlayerBodyCapture;
import java.util.Optional;
import net.minecraft.client.renderer.rendertype.PreparedRenderType;
import net.minecraft.client.renderer.rendertype.RenderType;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(RenderType.class)
public abstract class BodyRenderTypeMixin {
    @Inject(method = "prepare", at = @At("RETURN"), require = 1)
    private void crafterhunter$markBody(CallbackInfoReturnable<PreparedRenderType> callback) {
        PlayerBodyCapture.prepared((RenderType)(Object)this, callback.getReturnValue());
    }
    @Inject(method = "outline", at = @At("RETURN"), cancellable = true, require = 1)
    private void crafterhunter$bodyOutline(CallbackInfoReturnable<Optional<RenderType>> callback) {
        var outline = callback.getReturnValue();
        if (outline.isPresent()) callback.setReturnValue(Optional.of(
            PlayerBodyCapture.outline((RenderType)(Object)this, outline.get())));
    }
}

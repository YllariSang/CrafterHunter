package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.PlayerBodyCapture;
import net.minecraft.client.renderer.StagedVertexBuffer;
import net.minecraft.client.renderer.rendertype.PreparedRenderType;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(PreparedRenderType.class)
public abstract class PreparedBodyDrawMixin {
    @Inject(method = "drawFromBuffer(Lnet/minecraft/client/renderer/StagedVertexBuffer$ExecuteInfo;)V",
        at = @At("HEAD"), cancellable = true, require = 1)
    private void crafterhunter$excludeBody(StagedVertexBuffer.ExecuteInfo info, CallbackInfo callback) {
        if (PlayerBodyCapture.omit((PreparedRenderType)(Object)this)) callback.cancel();
    }
}

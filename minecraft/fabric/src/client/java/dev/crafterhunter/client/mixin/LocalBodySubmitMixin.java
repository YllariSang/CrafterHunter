package dev.crafterhunter.client.mixin;

import com.mojang.blaze3d.vertex.PoseStack;
import dev.crafterhunter.client.PlayerBodyCapture;
import net.minecraft.client.model.Model;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.client.renderer.entity.LivingEntityRenderer;
import net.minecraft.client.renderer.feature.ModelFeatureRenderer;
import net.minecraft.client.renderer.rendertype.RenderType;
import net.minecraft.client.renderer.texture.TextureAtlasSprite;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

@Mixin(LivingEntityRenderer.class)
public abstract class LocalBodySubmitMixin {
    @Redirect(method = "submit(Lnet/minecraft/client/renderer/entity/state/LivingEntityRenderState;Lcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/SubmitNodeCollector;Lnet/minecraft/client/renderer/state/level/CameraRenderState;)V",
        require = 1, at = @At(value = "INVOKE", target = "Lnet/minecraft/client/renderer/SubmitNodeCollector;submitModel(Lnet/minecraft/client/model/Model;Ljava/lang/Object;Lcom/mojang/blaze3d/vertex/PoseStack;Lnet/minecraft/client/renderer/rendertype/RenderType;IIILnet/minecraft/client/renderer/texture/TextureAtlasSprite;ILnet/minecraft/client/renderer/feature/ModelFeatureRenderer$CrumblingOverlay;)V"))
    private <S> void crafterhunter$bodyBatch(SubmitNodeCollector collector, Model<? super S> model,
            S state, PoseStack pose, RenderType type, int light, int overlay, int tint,
            TextureAtlasSprite sprite, int outline, ModelFeatureRenderer.CrumblingOverlay crumbling) {
        collector.submitModel(model, state, pose, PlayerBodyCapture.bodyType(model, state, type),
            light, overlay, tint, sprite, outline, crumbling);
    }
}

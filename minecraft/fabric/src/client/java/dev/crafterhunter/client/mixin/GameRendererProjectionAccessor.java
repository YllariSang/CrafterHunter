package dev.crafterhunter.client.mixin;

import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.ProjectionMatrixBuffer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/** The level projection buffer, so the projection uploaded from it can be read. */
@Mixin(GameRenderer.class)
public interface GameRendererProjectionAccessor {

    @Accessor("levelProjectionMatrixBuffer")
    ProjectionMatrixBuffer crafterhunter$levelProjectionBuffer();
}

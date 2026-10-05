package dev.crafterhunter.client.mixin;

import net.minecraft.client.renderer.Projection;
import net.minecraft.client.renderer.ProjectionMatrixBuffer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/**
 * The projection the level was last rendered with.
 *
 * <p>Minecraft keeps the matrix on the GPU: {@code GameRenderer} holds a
 * {@code ProjectionMatrixBuffer} whose authoritative copy is a graphics buffer. Reading
 * that back per capture would be a GPU stall for sixteen floats, so this reaches the
 * {@link Projection} the buffer last uploaded from - the same object the renderer used,
 * rather than a rebuild from the field of view.
 *
 * <p>A mixin accessor rather than reflection, so a rename breaks the build rather than
 * failing quietly at runtime. Callers treat an unreachable projection as absent and
 * record that, because a projection guessed from the field of view is indistinguishable
 * from a measured one unless its provenance is written down.
 */
@Mixin(ProjectionMatrixBuffer.class)
public interface LevelProjectionAccessor {

    @Accessor("lastUploadedProjection")
    Projection crafterhunter$lastUploadedProjection();
}

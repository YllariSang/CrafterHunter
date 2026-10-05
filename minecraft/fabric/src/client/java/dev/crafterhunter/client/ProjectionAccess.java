package dev.crafterhunter.client;

import dev.crafterhunter.client.mixin.GameRendererProjectionAccessor;
import dev.crafterhunter.client.mixin.LevelProjectionAccessor;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.Projection;

/**
 * Reads the projection the level was rendered with, or reports that it could not be.
 *
 * <p>Every failure returns null rather than a reconstructed matrix. A projection built
 * from the field of view would look exactly like a measured one in the capture metadata
 * unless the provenance were recorded, and a distance reconstructed from a guessed
 * projection is confidently wrong - which is worse than no distance at all.
 */
public final class ProjectionAccess {

    private ProjectionAccess() {
    }

    /** The level's projection, or null when it cannot be reached. */
    public static Projection levelProjection(GameRenderer renderer) {
        if (renderer == null) {
            return null;
        }
        try {
            Object holder = ((GameRendererProjectionAccessor) (Object) renderer)
                .crafterhunter$levelProjectionBuffer();
            if (holder == null) {
                return null;
            }
            return ((LevelProjectionAccessor) holder).crafterhunter$lastUploadedProjection();
        } catch (RuntimeException | LinkageError unreachable) {
            return null;
        }
    }
}

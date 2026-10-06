package dev.crafterhunter.client;

import dev.crafterhunter.client.mixin.CameraProjectionAccessor;
import net.minecraft.client.Camera;
import net.minecraft.client.renderer.Projection;

/**
 * Reads the projection the level is rendered with, or reports that it could not be.
 *
 * <p>The projection lives on the camera: {@code Camera} holds a {@link Projection}
 * configured each frame with the level's near, far and field of view, and
 * {@code extractRenderState} copies its matrix into the render state that
 * {@code renderLevel} uploads to the GPU. Reaching it there is not a preference but
 * the only thing that works. The earlier path read
 * {@code ProjectionMatrixBuffer.lastUploadedProjection}, and this game version's
 * bytecode shows why that can never return anything: the level upload goes through
 * {@code getBuffer(Matrix4f)}, whose first act is to set that field to null. The
 * field names were all correct; the value was structurally always null.
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
    public static Projection levelProjection(Camera camera) {
        if (camera == null) {
            return null;
        }
        try {
            return ((CameraProjectionAccessor) (Object) camera).crafterhunter$projection();
        } catch (RuntimeException | LinkageError unreachable) {
            return null;
        }
    }
}

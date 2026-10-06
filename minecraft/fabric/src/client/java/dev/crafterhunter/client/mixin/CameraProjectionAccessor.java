package dev.crafterhunter.client.mixin;

import net.minecraft.client.Camera;
import net.minecraft.client.renderer.Projection;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/**
 * The camera's projection: the level's near, far, field of view and matrix.
 *
 * <p>A mixin accessor rather than reflection, so a rename breaks the build rather than
 * failing quietly at runtime - which is how the previous chain survived a build while
 * being unreachable at runtime: the field names were right, but the value they read is
 * set to null by the very upload that uses it.
 */
@Mixin(Camera.class)
public interface CameraProjectionAccessor {

    @Accessor("projection")
    Projection crafterhunter$projection();
}

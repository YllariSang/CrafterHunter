package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.CameraFeed;
import dev.crafterhunter.client.CameraLink;
import net.minecraft.client.Camera;
import net.minecraft.client.Minecraft;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(Camera.class)
public abstract class CameraMixin {
    @Shadow
    protected abstract void setPosition(double x, double y, double z);

    @Shadow
    protected abstract void setRotation(float yawDegrees, float pitchDegrees);

    @Shadow
    public abstract Vec3 position();

    /**
     * Minecraft 26.1+ moved the vanilla camera transform from setup to
     * alignWithEntity. Applying at RETURN lets vanilla initialize all derived state
     * first, then replaces the final pose for this frame.
     */
    @Inject(method = "alignWithEntity(F)V", at = @At("RETURN"))
    private void crafterhunter$applyMhwPose(float partialTicks, CallbackInfo callback) {
        Minecraft minecraft = Minecraft.getInstance();
        Vec3 vanilla = position();
        CameraLink.Pose pose = CameraLink.instance().update(
            CameraFeed.latestFresh().orElse(null), minecraft.level,
            vanilla.x, vanilla.y, vanilla.z, System.nanoTime());
        if (pose != null) {
            setPosition(pose.x(), pose.y(), pose.z());
            setRotation(pose.yaw(), pose.pitch());
        }
    }

    @Inject(method = "calculateFov(F)F", at = @At("RETURN"), cancellable = true)
    private void crafterhunter$applyMhwFov(
        float partialTicks,
        CallbackInfoReturnable<Float> callback
    ) {
        CameraLink.Pose pose = CameraLink.instance().pose();
        if (pose != null && CameraFeed.latestFresh().isPresent()) {
            callback.setReturnValue(pose.fov());
        }
    }
}

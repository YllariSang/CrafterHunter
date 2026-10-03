package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.CameraFeed;
import dev.crafterhunter.client.CameraState;
import net.minecraft.client.Camera;
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

    /**
     * Minecraft 26.1+ moved the vanilla camera transform from setup to
     * alignWithEntity. Applying at RETURN lets vanilla initialize all derived state
     * first, then replaces the final pose for this frame.
     */
    @Inject(method = "alignWithEntity(F)V", at = @At("RETURN"))
    private void crafterhunter$applyMhwPose(float partialTicks, CallbackInfo callback) {
        CameraFeed.latestFresh().ifPresent(this::crafterhunter$applyPose);
    }

    @Inject(method = "calculateFov(F)F", at = @At("RETURN"), cancellable = true)
    private void crafterhunter$applyMhwFov(
        float partialTicks,
        CallbackInfoReturnable<Float> callback
    ) {
        CameraFeed.latestFresh().ifPresent(
            state -> callback.setReturnValue(state.verticalFovDegrees())
        );
    }

    private void crafterhunter$applyPose(CameraState state) {
        // The protocol defines one metre per unit; Minecraft uses one block per metre.
        setPosition(state.x(), state.y(), state.z());
        // Vanilla Fabric 26.2 exposes only yaw and pitch here. Calling this method
        // also refreshes Camera's quaternion, direction vectors, and dirty flags.
        setRotation(state.minecraftYawDegrees(), state.minecraftPitchDegrees());
    }
}

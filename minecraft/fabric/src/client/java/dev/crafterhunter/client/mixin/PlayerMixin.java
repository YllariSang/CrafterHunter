package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.PlayerFeed;
import dev.crafterhunter.client.PlayerLink;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/**
 * Aligns the Minecraft player with the host hunter.
 *
 * While the proxy holds, it owns position, facing, and movement: velocity and
 * accumulated fall distance are cleared every tick so vanilla physics cannot
 * apply a second move for one the host already made, and the player is placed
 * at the target after vanilla has finished its own tick for the frame.
 *
 * The release path is the same code with no work to do. When the feed goes
 * stale, the link is toggled off, the world changes, or the host player
 * disappears, {@link PlayerLink#update} returns null and this method does not
 * touch the player at all, so control returns to the keyboard on that tick
 * rather than being resumed by a later correction.
 */
@Mixin(LocalPlayer.class)
public abstract class PlayerMixin {
    @Inject(method = "tick", at = @At("TAIL"))
    private void crafterhunter$alignToHostPlayer(CallbackInfo callback) {
        Minecraft minecraft = Minecraft.getInstance();
        LocalPlayer self = (LocalPlayer) (Object) this;
        PlayerLink.Target target = PlayerLink.instance().update(
            PlayerFeed.latestFresh().orElse(null),
            minecraft.level,
            self.getX(),
            self.getY(),
            self.getZ(),
            System.nanoTime()
        );
        if (target == null) {
            return;
        }

        self.setPos(target.x(), target.y(), target.z());
        self.setYRot(target.yaw());
        self.setDeltaMovement(Vec3.ZERO);
        self.resetFallDistance();
    }
}

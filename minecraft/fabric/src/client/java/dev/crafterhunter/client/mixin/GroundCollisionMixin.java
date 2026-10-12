package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.GroundRuntime;
import java.util.List;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.level.Level;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.VoxelShape;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Entity movement uses this helper on both local and integrated-server players. */
@Mixin(Entity.class)
public abstract class GroundCollisionMixin {
    @Inject(method="collectCollidersIgnoringWorldBorder(Lnet/minecraft/world/entity/Entity;Lnet/minecraft/world/level/Level;Ljava/util/List;Lnet/minecraft/world/phys/AABB;)Ljava/util/List;",
        at=@At("RETURN"),cancellable=true)
    private static void crafterhunter$ground(Entity entity, Level level, List<VoxelShape> existing,
            AABB box, CallbackInfoReturnable<List<VoxelShape>> callback) {
        callback.setReturnValue(GroundRuntime.append(entity,level,box,callback.getReturnValue()));
    }
}

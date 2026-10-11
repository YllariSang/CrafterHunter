package dev.crafterhunter.client.mixin;

import net.minecraft.client.renderer.rendertype.RenderSetup;
import net.minecraft.client.renderer.rendertype.RenderType;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(RenderType.class)
public interface RenderTypeAccessor {
    @Accessor("state") RenderSetup crafterhunter$state();
    @Invoker("create") static RenderType crafterhunter$create(String name, RenderSetup setup) {
        throw new AssertionError("mixin invoker not installed");
    }
}

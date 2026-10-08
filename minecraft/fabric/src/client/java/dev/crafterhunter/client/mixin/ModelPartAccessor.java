package dev.crafterhunter.client.mixin;

import java.util.List;
import java.util.Map;
import net.minecraft.client.model.geom.ModelPart;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/** Read baked geometry, never screen-visible geometry. Version-pinned to 26.2. */
@Mixin(ModelPart.class)
public interface ModelPartAccessor {
    @Accessor("cubes") List<ModelPart.Cube> crafterhunter$cubes();
    @Accessor("children") Map<String, ModelPart> crafterhunter$children();
}

package dev.crafterhunter.client;

import dev.crafterhunter.client.mixin.RenderTypeAccessor;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.Set;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.client.Minecraft;
import net.minecraft.client.model.player.PlayerModel;
import net.minecraft.client.renderer.entity.state.AvatarRenderState;
import net.minecraft.client.renderer.rendertype.PreparedRenderType;
import net.minecraft.client.renderer.rendertype.RenderType;

/** Body-only capture exclusion; attachment submits and Minecraft's display stay intact. */
public final class PlayerBodyCapture {
    private static final Path ENABLED = FabricLoader.getInstance().getGameDir()
        .resolve("crafterhunter/player-body-separate.enabled");
    private static final Map<RenderType, RenderType> aliases = new IdentityHashMap<>();
    private static final Set<PreparedRenderType> bodyDraws = Collections.newSetFromMap(new IdentityHashMap<>());
    private static boolean separate, filteredPass;
    private PlayerBodyCapture() {}

    public static void begin() {
        aliases.clear(); bodyDraws.clear();
        var mc = Minecraft.getInstance();
        long now = System.nanoTime();
        separate = Files.isRegularFile(ENABLED) && PlayerModelExport.available(now)
            && mc.player != null && mc.getCameraEntity() == mc.player;
        // Poll/publish completed copies without issuing a read before terrain is drawn.
        boolean due = WorldCapture.instance().wantsCapture(now);
        filteredPass = separate && !mc.options.getCameraType().isFirstPerson() && due;
    }
    public static boolean filteredPass() { return filteredPass; }
    public static boolean excludedAtIssue() {
        return separate && (BodyCapturePass.capturing()
            || Minecraft.getInstance().options.getCameraType().isFirstPerson());
    }
    public static RenderType bodyType(Object model, Object state, RenderType original) {
        var mc = Minecraft.getInstance();
        if (!filteredPass || !(model instanceof PlayerModel) || !(state instanceof AvatarRenderState avatar)
                || mc.player == null || avatar.id != mc.player.getId()) return original;
        // Same setup/pipeline/textures, distinct batching identity. Do not retag
        // another player, armor model, held item, cape, or the whole entity.
        return alias(original);
    }
    private static RenderType alias(RenderType original) {
        return aliases.computeIfAbsent(original, type -> RenderTypeAccessor.crafterhunter$create(
            "crafterhunter_local_body", ((RenderTypeAccessor)(Object) type).crafterhunter$state()));
    }
    public static RenderType outline(RenderType type, RenderType outline) {
        return aliases.containsValue(type) ? alias(outline) : outline;
    }
    public static void prepared(RenderType type, PreparedRenderType prepared) {
        if (aliases.containsValue(type)) bodyDraws.add(prepared);
    }
    public static boolean omit(PreparedRenderType prepared) {
        return BodyCapturePass.capturing() && bodyDraws.contains(prepared);
    }
}

package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.CameraFeed;
import dev.crafterhunter.client.CameraLink;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.Hud;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Hud.class)
public abstract class HudMixin {
    @Inject(method = "extractRenderState", at = @At("TAIL"))
    private void crafterhunter$status(GuiGraphicsExtractor graphics, DeltaTracker delta,
                                     CallbackInfo callback) {
        Minecraft minecraft = Minecraft.getInstance();
        if (minecraft.player == null || ((Hud) (Object) this).isHidden()) return;
        CameraFeed.Diagnostics status = CameraFeed.diagnostics();
        boolean enabled = CameraLink.instance().enabled();
        String state = !enabled ? "OFF" : status.live() ? "LIVE" : "WAITING";
        graphics.fill(4, 4, 300, 42, 0xB0000000);
        graphics.text(minecraft.font, "MHW LINK: " + state + " | " + status.packetsPerSecond()
            + " pkt/s | seq " + status.sequence() + " | age " + status.ageMillis() + "ms",
            8, 8, status.live() && enabled ? 0xFF80FF80 : 0xFFFFCC80);
        graphics.text(minecraft.font, "F7: toggle camera | F8: re-anchor", 8, 20, 0xFFFFFFFF);
        graphics.text(minecraft.font, minecraft.isPaused()
            ? "Paused: resume world; F3+P disables focus pause"
            : "Camera only: move/rotate in MHW to test", 8, 32, 0xFFFFFFFF);
    }
}

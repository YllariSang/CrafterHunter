package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.CameraFeed;
import dev.crafterhunter.client.CameraLink;
import dev.crafterhunter.client.BlockAssetFeed;
import dev.crafterhunter.client.FrameCapture;
import dev.crafterhunter.client.LinkStatus;
import dev.crafterhunter.client.PlayerFeed;
import dev.crafterhunter.client.PlayerLink;
import java.util.Locale;
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
        BlockAssetFeed.refresh(minecraft);
        // The same telemetry the lines below draw, written where a tool can read it.
        // Measuring synchronization from a composited screenshot cannot say which frame it
        // came from; this can.
        LinkStatus.maybeWrite();
        if (minecraft.player == null || ((Hud) (Object) this).isHidden()) return;
        CameraFeed.Diagnostics status = CameraFeed.diagnostics();
        PlayerFeed.Diagnostics player = PlayerFeed.diagnostics();
        boolean enabled = CameraLink.instance().enabled();
        boolean playerEnabled = PlayerLink.instance().enabled();
        String state = !enabled ? "OFF" : status.live() ? "LIVE" : "WAITING";
        String playerState = !playerEnabled ? "OFF" : player.live() ? "LIVE" : "WAITING";
        PlayerLink.Target proxy = PlayerLink.instance().target();
        String proxyPosition = proxy == null ? "-"
            : String.format(Locale.ROOT, "%.1f %.1f %.1f", proxy.x(), proxy.y(), proxy.z());
        graphics.fill(4, 4, 300, 54, 0xB0000000);
        graphics.text(minecraft.font, "MHW LINK: " + state + " | " + status.packetsPerSecond()
            + " pkt/s | seq " + status.sequence() + " | age " + status.ageMillis() + "ms",
            8, 8, status.live() && enabled ? 0xFF80FF80 : 0xFFFFCC80);
        graphics.text(minecraft.font, "F7: camera | F8: re-anchor | F9: player proxy",
            8, 20, 0xFFFFFFFF);
        graphics.text(minecraft.font, minecraft.isPaused()
            ? "Paused: resume world; F3+P disables focus pause"
            : "Move and rotate in MHW to test both links", 8, 32, 0xFFFFFFFF);
        graphics.text(minecraft.font, "PLAYER: " + playerState + " | age " + player.ageMillis()
            + "ms | proxy " + proxyPosition,
            8, 44, player.live() && playerEnabled ? 0xFF80FF80 : 0xFFFFCC80);
        graphics.fill(4, 54, 460, 66, 0xB0000000);
        graphics.text(minecraft.font, FrameCapture.instance().hudLine(), 8, 58, 0xFF80C0FF);
    }
}

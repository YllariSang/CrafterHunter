package dev.crafterhunter.client.mixin;

import dev.crafterhunter.client.CameraLink;
import net.minecraft.client.KeyboardHandler;
import net.minecraft.client.Minecraft;
import net.minecraft.client.input.KeyEvent;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(KeyboardHandler.class)
public abstract class KeyboardMixin {
    @Inject(method = "keyPress", at = @At("HEAD"))
    private void crafterhunter$controls(long window, int action, KeyEvent key, CallbackInfo callback) {
        Minecraft minecraft = Minecraft.getInstance();
        if (action != 1 || window != minecraft.getWindow().handle()
            || minecraft.player == null || minecraft.gui.screen() != null) return;
        if (key.key() == 296) CameraLink.instance().toggle(); // GLFW_KEY_F7
        if (key.key() == 297) CameraLink.instance().reset();  // GLFW_KEY_F8
    }
}

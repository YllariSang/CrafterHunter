package dev.crafterhunter.client;

import net.fabricmc.api.ClientModInitializer;

public final class CrafterHunterClient implements ClientModInitializer {
    private BridgeClient bridgeClient;

    @Override
    public void onInitializeClient() {
        bridgeClient = new BridgeClient();
        bridgeClient.start();
        Runtime.getRuntime().addShutdownHook(
            new Thread(bridgeClient::close, "crafterhunter-shutdown")
        );
        System.out.println("[CrafterHunter] Fabric endpoint initialized");
    }
}

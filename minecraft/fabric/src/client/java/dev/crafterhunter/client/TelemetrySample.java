package dev.crafterhunter.client;

/** A telemetry sample stamped by the receive thread at the moment it arrived. */
interface TelemetrySample {
    long receivedAtNanos();

    int sequence();
}

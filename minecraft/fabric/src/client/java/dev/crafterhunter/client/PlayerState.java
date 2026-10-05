package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * A validated MHW hunter sample in the wire protocol's coordinate system.
 *
 * Position is the host world position in metres, which is hundreds of metres
 * from anything in a Minecraft world, so only differences between samples are
 * ever applied; {@link PlayerLink} owns that mapping. Rotation is carried as
 * the raw model quaternion for the same reason the camera payload does, and the
 * facing used by Minecraft comes from the one conversion both links share.
 */
public record PlayerState(
    double x,
    double y,
    double z,
    float quaternionX,
    float quaternionY,
    float quaternionZ,
    float quaternionW,
    float minecraftYawDegrees,
    int sequence,
    long receivedAtNanos
) implements TelemetrySample {
    static final int PAYLOAD_LENGTH = 28;
    private static final double MIN_QUATERNION_LENGTH_SQUARED = 1.0e-12;

    static PlayerState decode(byte[] payload, int sequence, long receivedAtNanos) {
        if (payload.length != PAYLOAD_LENGTH) {
            throw new IllegalArgumentException("invalid player payload length: " + payload.length);
        }

        ByteBuffer values = ByteBuffer.wrap(payload).order(ByteOrder.LITTLE_ENDIAN);
        float x = values.getFloat();
        float y = values.getFloat();
        float z = values.getFloat();
        float quaternionX = values.getFloat();
        float quaternionY = values.getFloat();
        float quaternionZ = values.getFloat();
        float quaternionW = values.getFloat();

        requireFinite(x, y, z, quaternionX, quaternionY, quaternionZ, quaternionW);

        double lengthSquared = quaternionX * (double) quaternionX
            + quaternionY * (double) quaternionY
            + quaternionZ * (double) quaternionZ
            + quaternionW * (double) quaternionW;
        if (lengthSquared < MIN_QUATERNION_LENGTH_SQUARED) {
            throw new IllegalArgumentException("player quaternion has zero length");
        }

        float inverseLength = (float) (1.0 / Math.sqrt(lengthSquared));
        quaternionX *= inverseLength;
        quaternionY *= inverseLength;
        quaternionZ *= inverseLength;
        quaternionW *= inverseLength;

        Rotation.EulerAngles euler = Rotation.toMinecraftEuler(
            quaternionX, quaternionY, quaternionZ, quaternionW
        );
        return new PlayerState(
            x, y, z,
            quaternionX, quaternionY, quaternionZ, quaternionW,
            euler.yawDegrees(),
            sequence,
            receivedAtNanos
        );
    }

    private static void requireFinite(float... values) {
        for (float value : values) {
            if (!Float.isFinite(value)) {
                throw new IllegalArgumentException("player payload contains a non-finite value");
            }
        }
    }
}

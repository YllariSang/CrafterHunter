package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** A validated MHW camera sample expressed in the wire protocol's coordinate system. */
public record CameraState(
    double x,
    double y,
    double z,
    float quaternionX,
    float quaternionY,
    float quaternionZ,
    float quaternionW,
    float verticalFovDegrees,
    float aspectRatio,
    float nearPlaneMetres,
    float farPlaneMetres,
    float minecraftYawDegrees,
    float minecraftPitchDegrees,
    float minecraftRollDegrees,
    int sequence,
    long receivedAtNanos
) implements TelemetrySample {
    static final int PAYLOAD_LENGTH = 44;
    private static final float MIN_FOV_RADIANS = (float) Math.toRadians(1.0);
    private static final float MAX_FOV_RADIANS = (float) Math.toRadians(179.0);
    private static final double MIN_QUATERNION_LENGTH_SQUARED = 1.0e-12;

    static CameraState decode(byte[] payload, int sequence, long receivedAtNanos) {
        if (payload.length != PAYLOAD_LENGTH) {
            throw new IllegalArgumentException("invalid camera payload length: " + payload.length);
        }

        ByteBuffer values = ByteBuffer.wrap(payload).order(ByteOrder.LITTLE_ENDIAN);
        float x = values.getFloat();
        float y = values.getFloat();
        float z = values.getFloat();
        float quaternionX = values.getFloat();
        float quaternionY = values.getFloat();
        float quaternionZ = values.getFloat();
        float quaternionW = values.getFloat();
        float verticalFovRadians = values.getFloat();
        float aspectRatio = values.getFloat();
        float nearPlaneMetres = values.getFloat();
        float farPlaneMetres = values.getFloat();

        requireFinite(
            x, y, z,
            quaternionX, quaternionY, quaternionZ, quaternionW,
            verticalFovRadians, aspectRatio, nearPlaneMetres, farPlaneMetres
        );
        if (verticalFovRadians < MIN_FOV_RADIANS || verticalFovRadians > MAX_FOV_RADIANS) {
            throw new IllegalArgumentException("camera vertical FOV is outside (1, 179) degrees");
        }
        if (aspectRatio <= 0.0f || nearPlaneMetres <= 0.0f || farPlaneMetres <= nearPlaneMetres) {
            throw new IllegalArgumentException("invalid camera projection values");
        }

        double lengthSquared = quaternionX * (double) quaternionX
            + quaternionY * (double) quaternionY
            + quaternionZ * (double) quaternionZ
            + quaternionW * (double) quaternionW;
        if (lengthSquared < MIN_QUATERNION_LENGTH_SQUARED) {
            throw new IllegalArgumentException("camera quaternion has zero length");
        }

        float inverseLength = (float) (1.0 / Math.sqrt(lengthSquared));
        quaternionX *= inverseLength;
        quaternionY *= inverseLength;
        quaternionZ *= inverseLength;
        quaternionW *= inverseLength;

        Rotation.EulerAngles euler = Rotation.toMinecraftEuler(
            quaternionX,
            quaternionY,
            quaternionZ,
            quaternionW
        );
        return new CameraState(
            x,
            y,
            z,
            quaternionX,
            quaternionY,
            quaternionZ,
            quaternionW,
            (float) Math.toDegrees(verticalFovRadians),
            aspectRatio,
            nearPlaneMetres,
            farPlaneMetres,
            euler.yawDegrees(),
            euler.pitchDegrees(),
            euler.rollDegrees(),
            sequence,
            receivedAtNanos
        );
    }

    private static void requireFinite(float... values) {
        for (float value : values) {
            if (!Float.isFinite(value)) {
                throw new IllegalArgumentException("camera payload contains a non-finite value");
            }
        }
    }
}

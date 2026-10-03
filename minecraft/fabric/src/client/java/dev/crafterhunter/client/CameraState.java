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
) {
    static final int PAYLOAD_LENGTH = 44;
    private static final float MIN_FOV_RADIANS = (float) Math.toRadians(1.0);
    private static final float MAX_FOV_RADIANS = (float) Math.toRadians(179.0);
    private static final double MIN_QUATERNION_LENGTH_SQUARED = 1.0e-12;
    private static final double GIMBAL_LOCK_EPSILON = 1.0e-6;

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

        EulerAngles euler = toMinecraftEuler(
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

    private static EulerAngles toMinecraftEuler(float qx, float qy, float qz, float qw) {
        // Extract a Y-X-Z rotation from the canonical camera quaternion. Minecraft's
        // Camera#setRotation builds the same order, with a 180-degree yaw offset and
        // an inverted pitch angle because its zero-yaw view points toward +Z.
        double r00 = 1.0 - 2.0 * (qy * (double) qy + qz * (double) qz);
        double r02 = 2.0 * (qx * (double) qz + qw * (double) qy);
        double r10 = 2.0 * (qx * (double) qy + qw * (double) qz);
        double r11 = 1.0 - 2.0 * (qx * (double) qx + qz * (double) qz);
        double r12 = 2.0 * (qy * (double) qz - qw * (double) qx);
        double r20 = 2.0 * (qx * (double) qz - qw * (double) qy);
        double r22 = 1.0 - 2.0 * (qx * (double) qx + qy * (double) qy);

        double xRadians = Math.asin(clamp(-r12, -1.0, 1.0));
        double cosineX = Math.cos(xRadians);
        double yRadians;
        double zRadians;
        if (Math.abs(cosineX) > GIMBAL_LOCK_EPSILON) {
            yRadians = Math.atan2(r02, r22);
            zRadians = Math.atan2(r10, r11);
        } else {
            // At +/-90 degrees of pitch, yaw and roll describe the same axis. Keep
            // all of that rotation in yaw to produce a stable camera pose.
            yRadians = Math.atan2(-r20, r00);
            zRadians = 0.0;
        }

        return new EulerAngles(
            wrapDegrees(180.0f - (float) Math.toDegrees(yRadians)),
            -(float) Math.toDegrees(xRadians),
            wrapDegrees((float) Math.toDegrees(zRadians))
        );
    }

    private static void requireFinite(float... values) {
        for (float value : values) {
            if (!Float.isFinite(value)) {
                throw new IllegalArgumentException("camera payload contains a non-finite value");
            }
        }
    }

    private static double clamp(double value, double minimum, double maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    private static float wrapDegrees(float degrees) {
        float wrapped = degrees % 360.0f;
        if (wrapped >= 180.0f) {
            wrapped -= 360.0f;
        }
        if (wrapped < -180.0f) {
            wrapped += 360.0f;
        }
        return wrapped;
    }

    private record EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees) {
    }
}

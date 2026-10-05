package dev.crafterhunter.client;

/**
 * Quaternion to Minecraft yaw, pitch, and roll.
 *
 * This used to live inside {@link CameraState}. The player proxy needs the very
 * same conversion, because a camera and an entity in Minecraft are rotated by
 * the same angles: a player facing the direction the host model faces must use
 * the mapping the camera already proved against the real game. Having one copy
 * is the point - two would drift the first time one of them is corrected.
 */
final class Rotation {
    private static final double GIMBAL_LOCK_EPSILON = 1.0e-6;

    private Rotation() {
    }

    /**
     * Extract a Y-X-Z rotation from the canonical world quaternion. Minecraft's
     * Camera#setRotation builds the same order, with a 180-degree yaw offset and
     * an inverted pitch angle because its zero-yaw view points toward +Z.
     */
    static EulerAngles toMinecraftEuler(float qx, float qy, float qz, float qw) {
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
            // all of that rotation in yaw to produce a stable pose.
            yRadians = Math.atan2(-r20, r00);
            zRadians = 0.0;
        }

        return new EulerAngles(
            wrapDegrees(180.0f - (float) Math.toDegrees(yRadians)),
            -(float) Math.toDegrees(xRadians),
            wrapDegrees((float) Math.toDegrees(zRadians))
        );
    }

    private static double clamp(double value, double minimum, double maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    static float wrapDegrees(float degrees) {
        float wrapped = degrees % 360.0f;
        if (wrapped >= 180.0f) {
            wrapped -= 360.0f;
        }
        if (wrapped < -180.0f) {
            wrapped += 360.0f;
        }
        return wrapped;
    }

    record EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees) {
    }
}

package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;

public final class GuestCameraChannelTest {
    private static void check(boolean value) { if (!value) throw new AssertionError(); }
    public static void main(String[] args) throws Exception {
        float[] view = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        float[] projection = {1,0,0,0,0,2,0,0,0,0,1,-1,0,0,0.1f,0};
        byte[] bytes = GuestCameraChannel.encode(123,456,7,2,new double[] {10,20,34},view,projection);
        check(bytes.length == 192);
        ByteBuffer b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        check(b.getInt(0)==0x43474843 && b.getInt(4)==1 && b.getLong(8)==123);
        check(b.getLong(16)==456 && b.getLong(24)==7 && b.getInt(32)==2 && b.getInt(36)==1);
        check(b.getDouble(56)==34 && b.getFloat(64)==1 && b.getFloat(148)==2);
        Files.write(Path.of(args[0]), bytes);
        try {
            GuestCameraChannel.encode(123,0,7,2,new double[] {10,20,34},view,projection);
            throw new AssertionError("zero token accepted");
        } catch (IllegalArgumentException expected) { }
        var ack = new GuestCameraChannel.Acknowledgement();
        ack.observe("active 456 123 9 7",456,123,100);
        check(ack.active(456,123,499_999_999L));
        ack.observe("active 456 123 9 7",456,123,490_000_000L);
        check(!ack.active(456,123,500_000_100L)); // duplicate cannot renew
        ack.observe("active 999 123 9 8",456,123,600_000_000L);
        check(!ack.active(456,123,600_000_000L));
        ack.observe("active 456 123 9 8",456,123,600_000_000L);
        check(ack.active(456,123,600_000_001L));
        ack.observe("released 456 123 9 8",456,123,600_000_002L);
        check(!ack.active(456,123,600_000_003L));
        System.out.println("guest camera Java identity/acknowledgement regression passed");
    }
}

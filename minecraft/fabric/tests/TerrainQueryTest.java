package dev.crafterhunter.client;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
public class TerrainQueryTest {
    static void check(boolean b, String why) { if (!b) throw new AssertionError(why); }
    static byte[] result(int id, int status) {
        return ByteBuffer.allocate(36).order(ByteOrder.LITTLE_ENDIAN).putInt(id).putInt(status).array();
    }
    static int request(TerrainQuery q, Path p, long now, long nonce) throws Exception {
        Files.writeString(p.resolve("terrain.request"), nonce+" 4000 1 2 3.5 -4 5 6.5\n");
        byte[] bytes=q.poll(now, 1000);
        check(bytes!=null,"no query");
        check(!Files.exists(p.resolve("terrain.request")),"request was not claimed");
        return ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getInt();
    }
    public static void main(String[] args) throws Exception {
        byte[] golden=HexFormat.of().parseHex("010000000000803f0000004000006040000080c00000a0400000d040");
        check(Arrays.equals(golden,TerrainQuery.encode(1,new float[]{1,2,3.5f,-4,5,6.5f})),"Rust/C# golden mismatch");
        Path p=Files.createTempDirectory("terrain-test");
        TerrainQuery q=new TerrainQuery(p);
        int id=request(q,p,1_000_000_000L,7);
        check(q.poll(1_100_000_000L,1000)==null,"more than one outstanding query");
        q.receive(result(id+1,1),1_100_000_000L);
        check(!Files.exists(p.resolve("terrain.reply")),"unrelated reply accepted");
        byte[] bad=result(id,2); bad[5]=1; q.receive(bad,1_100_000_000L);
        check(!Files.exists(p.resolve("terrain.reply")),"reserved bits accepted");
        q.receive(result(id,1),1_100_000_000L);
        check(Files.readString(p.resolve("terrain.reply")).equals("7 miss\n"),"miss lost");
        id=request(q,p,2_000_000_000L,8);
        q.receive(result(id,2),3_000_000_000L);
        check(Files.readString(p.resolve("terrain.reply")).equals("8 timeout\n"),"late hit accepted");
        id=request(q,p,4_000_000_000L,9);
        q.receive(result(id,0),4_100_000_000L);
        check(Files.readString(p.resolve("terrain.reply")).equals("9 unavailable\n"),"unavailable became miss");
        id=request(q,p,5_000_000_000L,10);
        byte[] hit=HexFormat.of().parseHex("07000000020000000000803f000000c000006040000000000000803f0000000000001000");
        ByteBuffer.wrap(hit).order(ByteOrder.LITTLE_ENDIAN).putInt(id);
        q.receive(hit,5_100_000_000L);
        check(Files.readString(p.resolve("terrain.reply")).equals("10 hit 1.0 -2.0 3.5 0.0 1.0 0.0 1048576\n"),"hit golden mismatch");
        request(q,p,6_000_000_000L,11); q.disconnect();
        check(Files.readString(p.resolve("terrain.reply")).equals("11 disconnected\n"),"disconnect kept hit");
        Files.writeString(p.resolve("terrain.request"),"12 500 1 2 3 1 1 3\n");
        check(q.poll(7_000_000_000L,1000)==null,"expired request sent");
        check(Files.readString(p.resolve("terrain.reply")).equals("12 invalid\n"),"expired request not refused");
        // Reproduce the live sequential-writer gap: the receive loop polls
        // before the writer has observed the reply and published its next ray.
        long now=10_000_000_000L;
        id=request(q,p,now,20);
        for(int ray=0;ray<5;ray++) {
            now+=60_000_000L;
            q.receive(result(id,1),now);
            check(q.poll(now,1000)==null,"empty post-reply poll produced a request");
            Files.writeString(p.resolve("terrain.request"),(21+ray)+" 4000 1 2 3.5 -4 5 6.5\n");
            check(q.poll(now+1_000_000L,1000)==null,"active poll is unbounded");
            now+=6_000_000L;
            byte[] next=q.poll(now,1000);
            check(next!=null,"sequential ray charged idle delay after reply");
            id=ByteBuffer.wrap(next).order(ByteOrder.LITTLE_ENDIAN).getInt();
            check(q.poll(now+6_000_000L,1000)==null,"burst allowed concurrent requests");
        }
        q.receive(result(id,1),now+60_000_000L);
        now+=3_000_000_000L;
        check(q.poll(now,1000)==null,"idle poll produced a request");
        Files.writeString(p.resolve("terrain.request"),"30 4000 1 2 3.5 -4 5 6.5\n");
        check(q.poll(now+6_000_000L,1000)==null,"burst did not return to idle polling");
        check(q.poll(now+50_000_000L,1000)!=null,"idle request not serviced");
        q.disconnect();
        now+=60_000_000L;
        check(q.poll(now,1000)==null,"disconnect poll produced a request");
        Files.writeString(p.resolve("terrain.request"),"31 4000 1 2 3.5 -4 5 6.5\n");
        check(q.poll(now+6_000_000L,1000)==null,"disconnect retained burst polling");
        check(q.poll(now+50_000_000L,1000)!=null,"post-disconnect idle query not serviced");
        q.disconnect();
        try(var paths=Files.list(p)) { for(Path f:paths.toList()) Files.delete(f); } Files.delete(p);
        System.out.println("Terrain golden bytes, bounded request, claim, stale/foreign replies and lifecycle passed");
    }
}

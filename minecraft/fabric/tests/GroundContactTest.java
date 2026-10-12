import dev.crafterhunter.client.GroundContact;
import dev.crafterhunter.client.GroundPatchChannel;
import dev.crafterhunter.client.GroundContact.*;
import net.minecraft.core.Direction;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.Shapes;
import java.util.List;

public class GroundContactTest {
    static void check(boolean v,String why) {if(!v)throw new AssertionError(why);}
    public static void main(String[] args) {
        Anchor a=new Anchor(1,2,3,80,-60,230,-5,-6,0);
        long now=1_000_000_000L;
        Hit[] hits={new Hit(-5,-6,0,0,1,0,now),new Hit(-5.25,-6,-.25,0,1,0,now),
            new Hit(-5.25,-6,.25,0,1,0,now),new Hit(-4.75,-6,-.25,0,1,0,now),new Hit(-4.75,-6,.25,0,1,0,now)};
        Patch p=GroundContact.flatPatch(a,hits,1,2,3,now);
        check(p!=null && p.top()==-60 && p.minX()==79.75 && p.maxZ()==230.25,"fixed XYZ mapping");
        var shape=Shapes.box(p.minX(),p.top()-.05,p.minZ(),p.maxX(),p.top(),p.maxZ());
        var body=new AABB(79.7,-59.9,229.7,80.3,-58.1,230.3);
        double fall=Shapes.collide(Direction.Axis.Y,body,List.of(shape),-.2);
        check(Math.abs(fall+.1)<1e-8,"vanilla did not stop falling at patch");
        check(Shapes.collide(Direction.Axis.Y,body,List.of(shape),.4)==.4,"jump blocked");
        check(Shapes.collide(Direction.Axis.Y,body.move(2,0,0),List.of(shape),-.2)==-.2,"infinite floor");
        check(Shapes.collide(Direction.Axis.Y,body,List.of(),-.2)==-.2,"empty scene oracle");
        check(GroundContact.flatPatch(a,hits,1,2,3,now+500_000_000L)==null,"stale floor");
        check(GroundContact.flatPatch(a,hits,9,2,3,now)==null,"generation changed");
        check(GroundContact.flatPatch(a,hits,1,9,3,now)==null,"anchor changed");
        check(GroundContact.flatPatch(a,hits,1,2,0,now)==null,"release kept floor");
        hits[0]=new Hit(-5,-6,0,0,.01,0,now);
        check(GroundContact.flatPatch(a,hits,1,2,3,now)==null,"old scaled normal accepted");
        hits[0]=new Hit(-5,-5.8,0,0,1,0,now);
        check(GroundContact.flatPatch(a,hits,1,2,3,now)==null,"step became flat floor");
        hits[0]=new Hit(-5,-6,0,.6,.8,0,now);
        check(GroundContact.flatPatch(a,hits,1,2,3,now)==null,"slope accepted by flat-only prototype");
        hits[0]=new Hit(-5,-6,0,0,1,0,now+1);
        check(GroundContact.flatPatch(a,hits,1,2,3,now)==null,"future receipt accepted");
        hits[0]=null;check(GroundContact.flatPatch(a,hits,1,2,3,now)==null,"missing ground fabricated");
        GroundPatchChannel channel=new GroundPatchChannel();
        StringBuilder message=new StringBuilder("1 10 1000 3 1 2 80 -60 230 -5 -6 0");
        double[][] points={{-5,0},{-5.25,-.25},{-5.25,.25},{-4.75,-.25},{-4.75,.25}};
        for(double[] point:points) message.append(" ").append(point[0]).append(" -6 ").append(point[1]).append(" 0 1 0 100");
        String text=message.toString();
        var frame=channel.decode(text,3,1,2,now,1050);
        check(frame!=null && frame.expiresNanos()==now+350_000_000L,"receipt/sampler ages not bounded");
        check(channel.decode(text,3,1,2,now+10,1050)==null,"duplicate renewed lease");
        check(new GroundPatchChannel().decode(text,3,1,2,now,1500)==null,"stale initial publication accepted");
        check(new GroundPatchChannel().decode(text,3,1,2,now,999)==null,"future wall publication accepted");
        check(new GroundPatchChannel().decode(text,4,1,2,now,1050)==null,"wrong ownership accepted");
        check(new GroundPatchChannel().decode(text+" 0",3,1,2,now,1050)==null,"extra fields accepted");
        System.out.println("Ground patch channel duplicate, age, ownership and malformed refusal passed");
        System.out.println("Flat patch mapping, lifecycle and actual Minecraft landing/jump/edge collision passed");
    }
}

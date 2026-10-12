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
        Hit[] strip={new Hit(-5,-6,0,0,1,0,now),
            new Hit(-5.25,-6,-.25,0,1,0,now),new Hit(-5.25,-6,.25,0,1,0,now),
            new Hit(-4.75,-6,-.25,0,1,0,now),new Hit(-4.75,-6,.25,0,1,0,now),
            new Hit(-4.5,-6,0,0,1,0,now),new Hit(-4.25,-6,-.25,0,1,0,now),
            new Hit(-4.25,-6,.25,0,1,0,now)};
        var cells=GroundContact.flatStrip(a,strip,1,2,3,now);
        check(cells!=null && cells.size()==2 && cells.get(0).maxX()==cells.get(1).minX(),"strip has gap");
        var shapes=cells.stream().map(c->Shapes.box(c.minX(),c.top()-.05,c.minZ(),c.maxX(),c.top(),c.maxZ())).toList();
        // Move across the seam and fully beyond the original cell's support.
        for(double x=0;x<=.7;x+=.05) {
            check(Math.abs(Shapes.collide(Direction.Axis.Y,body.move(x,0,0),shapes,-.2)+.1)<1e-8,"seam fall");
            check(Shapes.collide(Direction.Axis.X,body.move(x,-.1,0),shapes,.05)==.05,"seam blocked walking");
        }
        check(Shapes.collide(Direction.Axis.Y,body.move(.65,0,0),List.of(shape),-.2)==-.2,"old single cell mutant still supports far cell");
        check(Shapes.collide(Direction.Axis.Y,body.move(1.1,0,0),shapes,-.2)==-.2,"strip extended beyond sampled edge");
        check(Shapes.collide(Direction.Axis.Y,body,shapes,.4)==.4,"strip blocked jump");
        Hit saved=strip[7];strip[7]=null;
        check(GroundContact.flatStrip(a,strip,1,2,3,now)==null,"missing corner fabricated neighbour");
        strip[7]=new Hit(-4.25,-5.8,.25,0,1,0,now);
        check(GroundContact.flatStrip(a,strip,1,2,3,now)==null,"step flattened across cells");
        strip[7]=new Hit(-4.25,-6,.25,0,1,0,now-500_000_000L);
        check(GroundContact.flatStrip(a,strip,1,2,3,now)==null,"stale corner accepted");strip[7]=saved;
        StringBuilder two=new StringBuilder("2 20 1000 3 1 2 80 -60 230 -5 -6 0");
        for(Hit h:strip) two.append(" ").append(h.x()).append(" ").append(h.y()).append(" ").append(h.z()).append(" 0 1 0 100");
        var pair=new GroundPatchChannel().decode(two.toString(),3,1,2,now,1050);
        check(pair!=null && pair.patches().size()==2 && pair.expiresNanos()==now+350_000_000L,"v2 strip channel failed");
        check(new GroundPatchChannel().decode(two.toString(),3,1,2,now,1400)==null,"stale v2 accepted");
        check(new GroundPatchChannel().decode(two.toString().replace("2 20","1 20"),3,1,2,now,1050)==null,"v1 accepted strip layout");
        System.out.println("Ground patch channel duplicate, age, ownership and malformed refusal passed");
        System.out.println("Flat patch mapping, lifecycle and actual Minecraft landing/jump/edge collision passed");
    }
}

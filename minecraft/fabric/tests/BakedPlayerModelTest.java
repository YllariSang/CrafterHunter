import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.model.geom.builders.CubeDeformation;
import net.minecraft.client.model.geom.builders.LayerDefinition;
import net.minecraft.client.model.player.PlayerModel;
import net.minecraft.client.renderer.entity.state.AvatarRenderState;
import java.util.HashSet;
import java.util.Set;
import java.util.ArrayList;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import dev.crafterhunter.client.PlayerModelChannel;

/** Actual local 26.2 baked geometry/animation, no cameras or game launch. */
public class BakedPlayerModelTest {
    public static void main(String[] args) throws Exception {
        for (boolean slim : new boolean[]{false,true}) {
            var root=LayerDefinition.create(PlayerModel.createMesh(CubeDeformation.NONE,slim),64,64).bakeRoot();
            var model=new PlayerModel(root,slim);
            int[] cubes={0},faces={0};
            root.visit(new PoseStack(),(pose,path,index,cube)->{
                cubes[0]++;
                Set<String> normals=new HashSet<>();
                for(var face:cube.polygons) {
                    faces[0]++;
                    normals.add(face.normal().toString());
                    if(face.vertices().length!=4) throw new AssertionError("nonquad");
                    for(var v:face.vertices()) {
                        if(!Float.isFinite(v.worldX()) || v.u()<0 || v.u()>1 || v.v()<0 || v.v()>1)
                            throw new AssertionError("geometry/UV invalid");
                    }
                }
                if(normals.size()!=6) throw new AssertionError("missing side/back/top/bottom: "+path);
            });
            if(cubes[0]<12 || faces[0]!=cubes[0]*6) throw new AssertionError("incomplete player");
            var state=new AvatarRenderState();
            state.showHat=state.showJacket=state.showLeftSleeve=state.showRightSleeve=true;
            state.showLeftPants=state.showRightPants=true;
            model.setupAnim(state);
            float resting=model.leftLeg.xRot;
            state.walkAnimationPos=2; state.walkAnimationSpeed=1;
            model.setupAnim(state);
            if(model.leftLeg.xRot==resting) throw new AssertionError("walk pose unchanged");
            state.xRot=30; state.yRot=45;
            model.setupAnim(state);
            if(Math.abs(model.head.xRot-(float)Math.toRadians(30))>0.0001)
                throw new AssertionError("head pose not native animation");
            if(args.length==1) {
                var matrices=new ArrayList<float[]>();
                var visibility=new ArrayList<Boolean>();
                var vertices=new ArrayList<float[]>();
                root.visit(new PoseStack(),(pose,path,index,cube)->{
                    int bone=matrices.size();
                    matrices.add(pose.pose().get(new float[16])); visibility.add(true);
                    for(var polygon:cube.polygons) for(int corner:new int[]{0,1,2,0,2,3}) {
                        var v=polygon.vertices()[corner];
                        vertices.add(new float[]{v.worldX(),v.worldY(),v.worldZ(),v.u(),v.v(),bone});
                    }
                });
                var geometry=ByteBuffer.allocate(vertices.size()*24).order(ByteOrder.LITTLE_ENDIAN);
                for(var v:vertices) { for(int i=0;i<5;i++) geometry.putFloat(v[i]); geometry.putInt((int)v[5]); }
                byte[] skin=new byte[64*64*4];
                for(int i=3;i<skin.length;i+=4) skin[i]=(byte)255; // Synthetic texture, not skin readback proof.
                Path out=Path.of(args[0]);
                Files.write(out.resolve("player.asset"),PlayerModelChannel.asset(7,2,matrices.size(),64,64,geometry.array(),skin));
                Files.write(out.resolve("player.pose"),PlayerModelChannel.pose(7,2,1,-917.5,-60,348,matrices,visibility));
            }
            System.out.println("PASS actual 26.2 baked player slim="+slim+" cubes="+cubes[0]+" faces="+faces[0]+" walk/head pose");
        }
    }
}

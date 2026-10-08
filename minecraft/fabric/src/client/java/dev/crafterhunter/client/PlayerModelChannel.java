package dev.crafterhunter.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

/** Little-endian player-only wire contract shared with player_model.hpp. */
public final class PlayerModelChannel {
    private PlayerModelChannel() {}
    public static byte[] asset(long generation,long identity,int parts,int width,int height,
            byte[] geometry,byte[] skin) {
        if(generation==0 || identity<=0 || parts<1 || parts>64 || width<1 || height<1
                || width>256 || height>256 || geometry.length==0 || geometry.length%72!=0
                || geometry.length>4096*24 || skin.length!=width*height*4)
            throw new IllegalArgumentException("invalid player asset");
        ByteBuffer b=ByteBuffer.allocate(40+geometry.length+skin.length).order(ByteOrder.LITTLE_ENDIAN);
        b.putInt(0x4d504843).putInt(1).putLong(generation).putLong(identity);
        b.putInt(parts).putInt(geometry.length/24).putInt(width).putInt(height).put(geometry).put(skin);
        return b.array();
    }
    public static byte[] pose(long generation,long asset,long sequence,double x,double y,double z,
            List<float[]> matrices,List<Boolean> visibility) {
        if(generation==0 || asset<=0 || sequence<=0 || matrices.isEmpty() || matrices.size()>64
                || visibility.size()!=matrices.size() || !Double.isFinite(x) || !Double.isFinite(y) || !Double.isFinite(z))
            throw new IllegalArgumentException("invalid player pose");
        ByteBuffer b=ByteBuffer.allocate(64+matrices.size()*68).order(ByteOrder.LITTLE_ENDIAN);
        b.putInt(0x50504843).putInt(1).putLong(generation).putLong(asset).putLong(sequence);
        b.putDouble(x).putDouble(y).putDouble(z).putInt(matrices.size()).putInt(0);
        for(int i=0;i<matrices.size();i++) {
            float[] matrix=matrices.get(i);
            if(matrix.length!=16 || matrix[3]!=0 || matrix[7]!=0 || matrix[11]!=0 || matrix[15]!=1)
                throw new IllegalArgumentException("nonaffine player matrix");
            b.putInt(visibility.get(i)?1:0);
            for(float v:matrix) {
                if(!Float.isFinite(v)) throw new IllegalArgumentException("nonfinite player matrix");
                b.putFloat(v);
            }
        }
        return b.array();
    }
}

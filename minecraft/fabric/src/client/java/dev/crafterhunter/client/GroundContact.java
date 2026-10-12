package dev.crafterhunter.client;

/** Flat-ground geometry used by the opt-in runtime and headless solver tests.
 * A finite sampled patch is an approximation, never an infinite floor. */
public final class GroundContact {
    private GroundContact() { }
    public record Anchor(long generation, long epoch, long token,
            double guestX, double guestY, double guestZ,
            double hostX, double hostY, double hostZ) { }
    public record Hit(double x, double y, double z, double nx, double ny, double nz,
                      long receivedNanos) { }
    public record Patch(double minX, double top, double minZ, double maxX, double maxZ) { }

    /** Five hits: centre and the four corners of a 0.5m square, fixed XYZ mapping.
     * Callers must invalidate the anchor on release/scene changes and supply only
     * matched query replies. Missing/stale/steep/nonunit data yields no collider. */
    public static Patch flatPatch(Anchor a, Hit[] hits, long generation, long epoch,
                                  long token, long now) {
        if (a == null || hits == null || hits.length != 5 || a.generation <= 0 || a.epoch <= 0
                || a.token <= 0 || a.generation != generation || a.epoch != epoch || a.token != token)
            return null;
        if (!finite(a.guestX,a.guestY,a.guestZ,a.hostX,a.hostY,a.hostZ)) return null;
        double top = Double.POSITIVE_INFINITY, high = Double.NEGATIVE_INFINITY;
        for (Hit h : hits) {
            if (h == null || !finite(h.x,h.y,h.z,h.nx,h.ny,h.nz)
                    || now < h.receivedNanos || now - h.receivedNanos >= 500_000_000L) return null;
            double norm2=h.nx*h.nx+h.ny*h.ny+h.nz*h.nz;
            if (Math.abs(norm2-1) > .01 || h.ny < .999) return null;
            top=Math.min(top,h.y);high=Math.max(high,h.y);
        }
        if (high-top > .01) return null;
        Hit centre=hits[0];
        double[][] offsets={{-.25,-.25},{-.25,.25},{.25,-.25},{.25,.25}};
        for(int i=0;i<4;i++) {
            Hit h=hits[i+1];
            if(Math.abs(h.x-centre.x-offsets[i][0])>.001
                    || Math.abs(h.z-centre.z-offsets[i][1])>.001) return null;
        }
        double x=a.guestX+centre.x-a.hostX,z=a.guestZ+centre.z-a.hostZ;
        return new Patch(x-.25,a.guestY+top-a.hostY,z-.25,x+.25,z+.25);
    }
    /** Two adjacent cells along fixed host/guest +X, sharing their boundary rays.
     * All eight samples must agree; no interpolation over missing/nonflat cells. */
    public static java.util.List<Patch> flatStrip(Anchor a, Hit[] hits, long generation,
                                                 long epoch, long token, long now) {
        if(a==null || hits==null || hits.length!=8 || hits[0]==null || hits[5]==null)
            return null;
        Patch left=flatPatch(a,new Hit[]{hits[0],hits[1],hits[2],hits[3],hits[4]},
                            generation,epoch,token,now);
        Patch right=flatPatch(a,new Hit[]{hits[5],hits[3],hits[4],hits[6],hits[7]},
                             generation,epoch,token,now);
        if(left==null || right==null || Math.abs(hits[0].x-a.hostX)>.001
                || Math.abs(hits[0].z-a.hostZ)>.001
                || Math.abs(hits[5].x-hits[0].x-.5)>.001
                || Math.abs(hits[5].z-hits[0].z)>.001) return null;
        double low=Double.POSITIVE_INFINITY,high=Double.NEGATIVE_INFINITY;
        for(Hit h:hits) { low=Math.min(low,h.y); high=Math.max(high,h.y); }
        if(high-low>.01) return null;
        // Snap only validated sub-millimetre serialization differences at the seam.
        double x=a.guestX,z=a.guestZ,top=a.guestY+low-a.hostY;
        return java.util.List.of(new Patch(x-.25,top,z-.25,x+.25,z+.25),
                                 new Patch(x+.25,top,z-.25,x+.75,z+.25));
    }
    private static boolean finite(double... values) {
        for(double v:values) if(!Double.isFinite(v)) return false;
        return true;
    }
}

package dev.crafterhunter.client;

/** Immutable local sampler message; receipt age is never renewed by duplicate reads. */
public final class GroundPatchChannel {
    public record Frame(long sequence, GroundContact.Anchor anchor, java.util.List<GroundContact.Patch> patches,
                        long expiresNanos) {
        public GroundContact.Patch patch() { return patches.getFirst(); }
    }
    private long lastSequence;
    public Frame decode(String text, long token, long generation, long epoch,
                        long now, long wallMillis) {
        try {
            if(text.length()>4096) return null;
            String[] f=text.strip().split("\\s+");
            int count=f[0].equals("1")?5:f[0].equals("2")?8:0;
            if(count==0 || f.length!=12+count*7) return null;
            long sequence=Long.parseLong(f[1]), published=Long.parseLong(f[2]);
            long age=wallMillis-published;
            if(sequence<=lastSequence || age<0 || age>=500) return null;
            GroundContact.Anchor a=new GroundContact.Anchor(Long.parseLong(f[4]),Long.parseLong(f[5]),
                Long.parseLong(f[3]),Double.parseDouble(f[6]),Double.parseDouble(f[7]),Double.parseDouble(f[8]),
                Double.parseDouble(f[9]),Double.parseDouble(f[10]),Double.parseDouble(f[11]));
            GroundContact.Hit[] hits=new GroundContact.Hit[count];
            long oldest=0;
            for(int i=0;i<count;i++) {
                int j=12+i*7; long sampleAge=Long.parseLong(f[j+6]);
                if(sampleAge<0 || sampleAge>=500 || sampleAge+age>=500) return null;
                oldest=Math.max(oldest,sampleAge+age);
                hits[i]=new GroundContact.Hit(Double.parseDouble(f[j]),Double.parseDouble(f[j+1]),
                    Double.parseDouble(f[j+2]),Double.parseDouble(f[j+3]),Double.parseDouble(f[j+4]),
                    Double.parseDouble(f[j+5]),now-(sampleAge+age)*1_000_000L);
            }
            java.util.List<GroundContact.Patch> patches;
            if(count==5) {
                GroundContact.Patch patch=GroundContact.flatPatch(a,hits,generation,epoch,token,now);
                if(patch==null) return null;
                patches=java.util.List.of(patch);
            } else patches=GroundContact.flatStrip(a,hits,generation,epoch,token,now);
            if(patches==null) return null;
            lastSequence=sequence;
            return new Frame(sequence,a,patches,now+(500-oldest)*1_000_000L);
        } catch(IllegalArgumentException e) { return null; }
    }
}

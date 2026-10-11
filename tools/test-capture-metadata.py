#!/usr/bin/env python3
"""Headless regression for the actual metadata preparation block, no game files."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'minecraft/fabric/src/client/java/dev/crafterhunter/client/WorldCapture.java').read_text()
block = source.split('final long metadataStartNanos = System.nanoTime();', 1)[1].split(
    'final long metadataEndNanos = System.nanoTime();', 1)[0]
sha = source.split('    private static String sha256(byte[] bytes) {', 1)[1].split(
    '\n    private long lastIdentity;', 1)[0]
fixture = '''
import java.util.Arrays;
public class MetadataProbe {
    private static int scans;
    private static String describe(int w,int h,long id,long now,byte[] colour,byte[] depth,boolean wantDepth) {
        scans++;
        return "identity="+id+"\\n";
    }
    private static String sha256(byte[] bytes) {SHA_BODY
    private static String prepare(boolean streaming, boolean wantColour, boolean wantDepth, byte[] colour, byte[] depth) {
        int width=949,height=509;long identity=17,nowNanos=100;
        BLOCK
        return metadata;
    }
    public static void main(String[] args) {
        byte[] colour=new byte[949*509*4],depth=new byte[colour.length];
        Arrays.fill(colour,(byte)31);Arrays.fill(depth,(byte)42);
        byte[] originalColour=colour.clone(),originalDepth=depth.clone();
        if(!prepare(true,true,true,colour,depth).isEmpty() || scans!=0)
            throw new AssertionError("stream performed unused diagnostic work");
        String archive=prepare(false,true,true,colour,depth);
        if(scans!=1 || !archive.startsWith("identity=17\\n")
            || !archive.contains("colourSha256="+sha256(colour)+"\\n")
            || !archive.contains("depthSha256="+sha256(depth)+"\\n"))
            throw new AssertionError("archive lost capture identity/hash binding");
        String colourOnly=prepare(false,true,false,colour,depth);
        String depthOnly=prepare(false,false,true,colour,depth);
        if(scans!=3 || colourOnly.contains("depthSha256") || depthOnly.contains("colourSha256"))
            throw new AssertionError("unrequested attachment hashed");
        if(!Arrays.equals(colour,originalColour) || !Arrays.equals(depth,originalDepth))
            throw new AssertionError("publication pixels altered");
        System.out.println("PASS: live stream omits unused metadata; archives retain requested hashes/identity; pixels unchanged");
    }
}
'''.replace('SHA_BODY', sha)
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    java = tmp/'MetadataProbe.java'
    def run(fragment):
        java.write_text(fixture.replace('BLOCK', fragment))
        subprocess.run(['javac','--release','25','-Xlint:all','-Werror','-d',str(tmp),str(java)],check=True)
        return subprocess.run(['java','-cp',str(tmp),'MetadataProbe'],capture_output=True,text=True)
    result = run(block)
    assert result.returncode == 0, result.stderr
    print(result.stdout.strip())
    assert block.count('if (!streaming)') == 1
    mutant = run(block.replace('if (!streaming)', 'if (true)'))
    assert mutant.returncode != 0, 'original unconditional stream metadata work survived'
    print('PASS: original unconditional diagnostic-work mutant rejected')

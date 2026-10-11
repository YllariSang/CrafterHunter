// Execute the production HLSL plane validator off-target through minimal vector
// shims. The test driver extracts its body; no second slope implementation.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

struct bool2 { bool x,y; };
struct bool4 { bool x,y,z,w; };
bool any(bool2 v) { return v.x || v.y; }
bool any(bool4 v) { return v.x || v.y || v.z || v.w; }
bool all(bool4 v) { return v.x && v.y && v.z && v.w; }
struct uint2 {
    unsigned x,y;
    uint2(unsigned a,unsigned b):x(a),y(b) {}
};
uint2 operator+(uint2 v,unsigned s) { return {v.x+s,v.y+s}; }
bool2 operator<(uint2 v,unsigned s) { return {v.x<s,v.y<s}; }
bool2 operator>=(uint2 a,uint2 b) { return {a.x>=b.x,a.y>=b.y}; }
struct int2 {
    int x,y;
    int2(int a,int b):x(a),y(b) {}
    explicit int2(uint2 v):x(v.x),y(v.y) {}
};
int2 operator+(int2 a,int2 b) { return {a.x+b.x,a.y+b.y}; }
struct int3 { int2 xy; int z; int3(int2 v,int a):xy(v),z(a) {} };
struct float2 {
    float x,y;
    float2(float s=0):x(s),y(s) {}
    float2(float a,float b):x(a),y(b) {}
};
float2 operator*(float2 v,float s) { return {v.x*s,v.y*s}; }
float2 abs(float2 v) { return {std::abs(v.x),std::abs(v.y)}; }
float dot(float2 a,float2 b) { return a.x*b.x+a.y*b.y; }
struct float4 {
    float x,y,z,w;
    float4(float a,float b,float c,float d):x(a),y(b),z(c),w(d) {}
};
float4 operator+(float a,float4 b) { return {a+b.x,a+b.y,a+b.z,a+b.w}; }
float4 operator-(float4 a,float4 b) { return {a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w}; }
bool4 operator<=(float4 v,float s) { return {v.x<=s,v.y<=s,v.z<=s,v.w<=s}; }
bool4 operator>(float4 v,float s) { return {v.x>s,v.y>s,v.z>s,v.w>s}; }
float4 abs(float4 v) { return {std::abs(v.x),std::abs(v.y),std::abs(v.z),std::abs(v.w)}; }
bool4 isfinite(float4 v) { return {std::isfinite(v.x),std::isfinite(v.y),std::isfinite(v.z),std::isfinite(v.w)}; }
bool isfinite(float v) { return std::isfinite(v); }
using std::abs;
using std::max;
unsigned width=5,height=5;
struct { float y=0; } mapping;
struct {
    std::vector<float> values=std::vector<float>(25);
    float Load(int3 p) const { return values.at(p.xy.y*width+p.xy.x); }
} guestDepth;

#include "paired-plane-shader-body.inc"

int failures=0;
void check(bool ok,const char* what) {
    if(!ok) { ++failures; std::printf("FAIL: %s\n",what); }
}
void plane(float centre,float dx,float dy) {
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        guestDepth.values[y*width+x]=centre+dx*(int(x)-2)+dy*(int(y)-2);
}
float2 slope() { return PairedPlaneSlope({2,2},guestDepth.values[12]); }
bool zero(float2 v) { return v.x==0 && v.y==0; }
int main() {
    // Captured floor's measured source-depth slope. Compare a magnified plane
    // against native near/far planes in HOST depth, not MC depth against MHW.
    plane(.014f,0,-8.3201383e-5f);
    const auto measured=slope();
    check(std::abs(measured.y+8.3201383e-5f)<1e-9f,"measured floor slope survives float readback");
    unsigned oldWrong=0;
    for(int side:{-1,1}) for(unsigned i=0;i<64;++i) {
        const float offset=(i+.5f)/64-.5f;
        const float centre=guestDepth.values[12];
        const float trueCorner=centre-8.3201383e-5f*offset;
        const float corner=centre+measured.y*offset;
        // The baseline's recovered native projection maps this source plane's
        // window depth approximately as A + 3.2*d (all in host clip space).
        const float nativeGuest=.0001456f+3.2f*trueCorner;
        const float host=nativeGuest+side*1e-5f;
        const bool expected=side<0;
        check((.0001456f+3.2f*corner>host+1e-6f)==expected,
              "tilted footprint preserves continuous strict host occlusion");
        oldWrong+=(.0001456f+3.2f*centre>host+1e-6f)!=expected;
    }
    check(oldWrong>50,"old flat footprint reproduces alternating visibility bands");
    for(auto gradient:{float2(.0007f,.0011f),float2(-.0007f,-.0011f),float2(0,0)}) {
        plane(.02f,gradient.x,gradient.y);const auto actual=slope();
        check(std::abs(actual.x-gradient.x)<2e-9f && std::abs(actual.y-gradient.y)<2e-9f,
              "valid planes retain both gradient axes/signs");
        for(auto xy:{uint2(0,2),uint2(4,2),uint2(2,0),uint2(2,4)})
            check(zero(PairedPlaneSlope(xy,guestDepth.values[xy.y*width+xy.x])),
                  "source border uses flat fallback without out-of-bounds loads");
    }
    // Every neighbour matters, including diagonals (a saddle can pass only the
    // axial checks). Invalid/clear data cannot manufacture a surface slope.
    for(unsigned y=1;y<=3;++y) for(unsigned x=1;x<=3;++x) {
        if(x==2 && y==2) continue;
        for(float bad:{0.f,-.01f,1.01f,std::numeric_limits<float>::quiet_NaN(),
                       std::numeric_limits<float>::infinity()}) {
            plane(.02f,.0001f,-.0002f);guestDepth.values[y*width+x]=bad;
            check(zero(slope()),"invalid axial/diagonal sample falls back");
        }
        plane(.02f,.0001f,-.0002f);guestDepth.values[y*width+x]+=.00001f;
        check(zero(slope()),"depth edge or nonplanar diagonal falls back");
    }
    for(float step:{-.005f,.005f}) {
        plane(.02f,0,0);
        for(unsigned y=0;y<height;++y) guestDepth.values[y*width+3]+=step;
        check(zero(slope()),"foreground/background depth step cannot tilt across an edge");
    }
    if(failures) return 1;
    std::puts("PASS: production HLSL slope body fixes tilted-plane bands; all 8 neighbours, edges, borders and strict occlusion checked");
}

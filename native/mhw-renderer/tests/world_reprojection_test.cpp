#include "world_reprojection.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace crafterhunter::reprojection;
int main(int argc,char** argv) {
    if(argc==23) {
        Matrix projection{},inv{};
        for(unsigned i=0;i<16;++i) projection[i]=std::strtod(argv[7+i],nullptr);
        Vector eye{};
        if(!inverse(projection,inv) || !reconstruct(inv,ClipDepth::ZeroToOne,0,
                std::strtoul(argv[2],nullptr,10),std::strtoul(argv[3],nullptr,10),
                std::strtoul(argv[4],nullptr,10),std::strtoul(argv[5],nullptr,10),
                std::strtod(argv[6],nullptr),eye)) return 1;
        std::printf("%.12f\n",-eye[2]); return 0;
    }
    assert(argc==1);
    const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    Matrix eyeWorld{};
    assert(eyeToWorld(identity,{917.546812345,-58.38,343.1757},eyeWorld));
    const auto worldPoint=transform(eyeWorld,{0,0,-5,1});
    assert(worldPoint[0]==917.546812345 && worldPoint[2]==338.1757);
    auto viewRotation=identity; viewRotation[0]=0; viewRotation[2]=-1; viewRotation[8]=1; viewRotation[10]=0;
    assert(eyeToWorld(viewRotation,{10,20,30},eyeWorld));
    const auto turned=transform(eyeWorld,{0,0,-5,1});
    assert(turned[0]==15 && turned[1]==20 && turned[2]==30);
    viewRotation[3]=1; assert(!eyeToWorld(viewRotation,{10,20,30},eyeWorld));
    // Actual recorded Minecraft projection coefficients (949x1028, FOV 70).
    Matrix p{1.5470351,0,0,0,0,1.428148,0,0,0,0,4.883051e-5,-1,0,0,0.05000244,0},inv{};
    assert(inverse(p,inv));
    Vector eye{}; HostPixel host{};
    assert(reconstruct(inv,ClipDepth::ZeroToOne,0,949,1028,474,514,0.01,eye));
    const double expected=0.05000244/(0.01+4.883051e-5);
    assert(std::abs(-eye[2]-expected)<1e-9);
    assert(project(eye,identity,p,host));
    assert(std::abs(host.u-474.5/949)<1e-12 && std::abs(host.v-(1-514.5/1028))<1e-12);
    assert(std::abs(host.depth-0.01)<1e-12);
    // Source-cell footprint (the VS contract): a flat surface must cover the
    // source cell after projection, not remain four host pixels at every size.
    // The former fixed-host footprint leaves ~4.09 px gaps at 949 -> 1920.
    for(const auto dimensions : {std::array<unsigned,4>{949,1028,1920,1080},
            std::array<unsigned,4>{1920,1080,949,1028},
            std::array<unsigned,4>{7,5,1920,1080},
            std::array<unsigned,4>{1,1,1,1}}) {
        const double w=dimensions[0],h=dimensions[1];
        const auto corner=[&](double x,double y,const Matrix& mappingMatrix) {
            auto e=transform(inv,{2*x/w-1,2*y/h-1,0.01,1});
            for(unsigned i=0;i<3;++i) e[i]/=e[3];
            e[3]=1;
            HostPixel result{}; assert(project(e,mappingMatrix,p,result)); return result;
        };
        const double x=std::min(4.0,w-1),y=std::min(4.0,h-1);
        const double left=std::max(0.0,x+0.5-2),right=std::min(w,x+0.5+2);
        const double bottom=std::max(0.0,y+0.5-2),top=std::min(h,y+0.5+2);
        auto lo=corner(left,bottom,identity),hi=corner(right,top,identity);
        assert(std::abs((hi.u-lo.u)*dimensions[2]-(right-left)/w*dimensions[2])<1e-9);
        assert(std::abs((lo.v-hi.v)*dimensions[3]-(top-bottom)/h*dimensions[3])<1e-9);
        if(w>8) {
            // Equal-depth neighboring source cells share the same projected edge.
            auto next=corner(x+4+0.5-2,bottom,identity);
            auto edge=corner(right,bottom,identity);
            assert(std::abs(next.u-edge.u)<1e-12);
        }
    }
    // Moving the host view closer changes footprint size, not sample count.
    auto nearer=identity; nearer[14]=1;
    const auto footprint=[&](const Matrix& mappingMatrix) {
        HostPixel ends[2];
        for(unsigned i=0;i<2;++i) {
            auto e=transform(inv,{2*(474.5+(i ? 2:-2))/949-1,0,0.01,1});
            for(unsigned j=0;j<3;++j) e[j]/=e[3];
            e[3]=1;
            assert(project(e,mappingMatrix,p,ends[i]));
        }
        return ends[1].u-ends[0].u;
    };
    assert(footprint(nearer)>footprint(identity));
    // Off-axis pixel centres and bottom-up/top-down conversion cannot cancel unnoticed.
    assert(reconstruct(inv,ClipDepth::ZeroToOne,0,949,1028,800,900,0.01,eye));
    assert(eye[0]>0 && eye[1]>0 && project(eye,identity,p,host));
    assert(std::abs(host.u-800.5/949)<1e-12 && std::abs(host.v-(1-900.5/1028))<1e-12);
    auto translated=identity; translated[12]=1;
    HostPixel shifted{}; assert(project(eye,translated,p,shifted) && shifted.u>host.u);
    auto rotated=identity; rotated[0]=-1; rotated[10]=-1;
    assert(!project(eye,rotated,p,shifted)); // behind host eye
    translated[12]=10000; assert(!project(eye,translated,p,shifted)); // off-screen
    Vector unchanged{9,8,7,1};
    assert(!reconstruct(inv,ClipDepth::ZeroToOne,0,949,1028,474,514,0,unchanged));
    assert(unchanged[0]==9);
    assert(!reconstruct(inv,ClipDepth::ZeroToOne,0,949,1028,949,514,0.01,eye));
    assert(!reconstruct(inv,ClipDepth::ZeroToOne,0,0,1028,0,514,0.01,eye));
    assert(!reconstruct(inv,ClipDepth::ZeroToOne,0,949,1028,474,514,-0.1,eye));
    assert(!reconstruct(inv,ClipDepth::ZeroToOne,0,949,1028,474,514,std::numeric_limits<double>::quiet_NaN(),eye));
    Matrix zero{}; assert(!inverse(zero,inv));
    auto invalid=p; invalid[0]=std::numeric_limits<double>::infinity(); assert(!inverse(invalid,inv));
    invalid=identity; invalid[15]=std::numeric_limits<double>::quiet_NaN();
    assert(!project(eye,invalid,p,host));
    assert(visible(0.02,0.01) && !visible(0.01,0.02) && !visible(0.01,0.01));
    assert(visible(0.01,0) && !visible(0,0) && !visible(2,0));
    assert(!visible(0.01,std::numeric_limits<double>::quiet_NaN()));
    assert(!visible(0.01,0,-1));
    // Classic GL forward-Z projection requires the explicit window-to-NDC remap.
    Matrix classic{1,0,0,0,0,1,0,0,0,0,-1.002002002,-1,0,0,-0.2002002002,0};
    assert(inverse(classic,inv));
    assert(reconstruct(inv,ClipDepth::MinusOneToOne,1,1,1,0,0,0.5,eye));
    assert(std::abs(-eye[2]-0.1998001998)<1e-9);
    assert(!reconstruct(inv,ClipDepth::MinusOneToOne,1,1,1,0,0,1,eye));
    std::puts("PASS: projection inversion, pixel centres, row orientation, transforms, clip bounds, clear/invalid refusal and host-space occlusion");
}

#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace crafterhunter::reprojection {
using Matrix=std::array<double,16>; // column-major; column vectors
using Vector=std::array<double,4>;
enum class ClipDepth { ZeroToOne, MinusOneToOne };
inline bool finite(const Matrix& m) {
    for(double value:m) if(!std::isfinite(value)) return false;
    return true;
}
inline Vector transform(const Matrix& m,const Vector& p) {
    Vector out{};
    for(unsigned row=0;row<4;++row)
        for(unsigned col=0;col<4;++col) out[row]+=m[col*4+row]*p[col];
    return out;
}
inline bool inverse(const Matrix& m,Matrix& result) {
    if(!finite(m)) return false;
    double a[4][8]{};
    for(unsigned row=0;row<4;++row) {
        for(unsigned col=0;col<4;++col) a[row][col]=m[col*4+row];
        a[row][row+4]=1;
    }
    for(unsigned col=0;col<4;++col) {
        unsigned pivot=col;
        for(unsigned row=col+1;row<4;++row)
            if(std::abs(a[row][col])>std::abs(a[pivot][col])) pivot=row;
        if(std::abs(a[pivot][col])<1e-12) return false;
        for(unsigned j=0;j<8;++j) std::swap(a[pivot][j],a[col][j]);
        const double scale=a[col][col];
        for(unsigned j=0;j<8;++j) a[col][j]/=scale;
        for(unsigned row=0;row<4;++row) if(row!=col) {
            const double factor=a[row][col];
            for(unsigned j=0;j<8;++j) a[row][j]-=factor*a[col][j];
        }
    }
    Matrix candidate{};
    for(unsigned row=0;row<4;++row)
        for(unsigned col=0;col<4;++col) candidate[col*4+row]=a[row][col+4];
    if(!finite(candidate)) return false;
    result=candidate; return true;
}
// Input integer row is the RAW bottom-up attachment row, not a screen Y.
// Clip mapping and clear value must be established externally; never inferred
// from a plausible distance. Output is guest eye space, looking down -Z.
inline bool reconstruct(const Matrix& inverseProjection,ClipDepth mapping,double clearDepth,
        std::uint32_t width,std::uint32_t height,std::uint32_t x,std::uint32_t y,
        double depth,Vector& eye) {
    if(!finite(inverseProjection) || !width || !height || x>=width || y>=height
            || !std::isfinite(clearDepth) || clearDepth<0 || clearDepth>1
            || !std::isfinite(depth) || depth<0 || depth>1 || depth==clearDepth) return false;
    if(mapping!=ClipDepth::ZeroToOne && mapping!=ClipDepth::MinusOneToOne) return false;
    const double z=mapping==ClipDepth::ZeroToOne ? depth : depth*2-1;
    Vector candidate=transform(inverseProjection,{2*(x+0.5)/width-1,2*(y+0.5)/height-1,z,1});
    if(!std::isfinite(candidate[3]) || std::abs(candidate[3])<1e-12) return false;
    for(unsigned i=0;i<3;++i) {
        candidate[i]/=candidate[3];
        if(!std::isfinite(candidate[i])) return false;
    }
    if(candidate[2]>=0) return false;
    candidate[3]=1; eye=candidate; return true;
}
struct HostPixel { double u{},v{},depth{}; }; // host top-down UV and host window depth
// eyeToHostWorld must include the VERIFIED guest view inverse and world-anchor
// conversion. Host VP must be the current draw's matrix, not a lagged CPU guess.
inline bool project(const Vector& eye,const Matrix& eyeToHostWorld,
        const Matrix& hostViewProjection,HostPixel& output) {
    if(!finite(eyeToHostWorld) || !finite(hostViewProjection) || eye[3]!=1) return false;
    for(double value:eye) if(!std::isfinite(value)) return false;
    const auto clip=transform(hostViewProjection,transform(eyeToHostWorld,eye));
    if(!std::isfinite(clip[3]) || clip[3]<=1e-12) return false;
    const double x=clip[0]/clip[3],y=clip[1]/clip[3],z=clip[2]/clip[3];
    if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)
            || x< -1 || x>1 || y< -1 || y>1 || z<0 || z>1) return false;
    output={(x+1)*0.5,(1-y)*0.5,z}; return true;
}
// Both depths here are in HOST reversed-Z window space, never MC linear depth.
inline bool visible(double reprojectedHostDepth,double sceneDepth,double epsilon=1e-6) {
    return std::isfinite(reprojectedHostDepth) && std::isfinite(sceneDepth)
        && std::isfinite(epsilon) && epsilon>=0
        && reprojectedHostDepth>0 && reprojectedHostDepth<=1
        && sceneDepth>=0 && sceneDepth<=1
        && reprojectedHostDepth>sceneDepth+epsilon;
}
}

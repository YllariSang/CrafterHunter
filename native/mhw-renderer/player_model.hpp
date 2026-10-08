#pragma once
#include "world_frame.hpp"

namespace crafterhunter::player {
constexpr unsigned MaxParts=64, MaxVertices=4096, MaxSkin=256;
struct Vertex { float x,y,z,u,v; std::uint32_t bone; };
static_assert(sizeof(Vertex)==24);
struct Asset {
    std::uint64_t generation{},identity{};
    unsigned parts{},width{},height{};
    std::vector<Vertex> vertices;
    std::vector<std::uint8_t> skin;
};
struct Pose {
    std::uint64_t generation{},asset{},sequence{};
    std::array<double,3> feet{};
    struct Bone { std::array<float,16> matrix; std::uint32_t visible; };
    std::vector<Bone> bones;
};
inline bool bytes(const char* path,std::size_t limit,std::vector<std::uint8_t>& out) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f) return false;
    const auto n=f.tellg(); if(n<0 || std::uint64_t(n)>limit) return false;
    f.seekg(0); std::vector<std::uint8_t> b(static_cast<std::size_t>(n));
    if(!f.read(reinterpret_cast<char*>(b.data()),b.size())) return false;
    out=std::move(b); return true;
}
inline bool readAsset(const char* path,Asset& out) {
    using namespace world;
    std::vector<std::uint8_t> b;
    if(!bytes(path,40+MaxVertices*24+MaxSkin*MaxSkin*4,b) || b.size()<40) return false;
    const auto p=b.data();
    if(u32(p)!=0x4d504843 || u32(p+4)!=1) return false;
    Asset a; a.generation=u64(p+8); a.identity=u64(p+16); a.parts=u32(p+24);
    const auto count=u32(p+28); a.width=u32(p+32); a.height=u32(p+36);
    if(!a.generation || !a.identity || !a.parts || a.parts>MaxParts || !count || count>MaxVertices || count%3
            || !a.width || !a.height || a.width>MaxSkin || a.height>MaxSkin
            || b.size()!=40+std::size_t(count)*24+std::size_t(a.width)*a.height*4) return false;
    for(unsigned i=0;i<count;++i) {
        const auto v=p+40+i*24;
        Vertex vertex{f32(v),f32(v+4),f32(v+8),f32(v+12),f32(v+16),u32(v+20)};
        if(!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)
                || !std::isfinite(vertex.u) || !std::isfinite(vertex.v) || vertex.bone>=a.parts) return false;
        a.vertices.push_back(vertex);
    }
    a.skin.assign(b.begin()+40+count*24,b.end()); out=std::move(a); return true;
}
inline bool readPose(const char* path,Pose& out) {
    using namespace world;
    std::vector<std::uint8_t> b;
    if(!bytes(path,64+MaxParts*68,b) || b.size()<64) return false;
    const auto p=b.data();
    if(u32(p)!=0x50504843 || u32(p+4)!=1 || u32(p+60)) return false;
    Pose s; s.generation=u64(p+8); s.asset=u64(p+16); s.sequence=u64(p+24);
    const auto count=u32(p+56);
    if(!s.generation || !s.asset || !s.sequence || !count || count>MaxParts || b.size()!=64+count*68) return false;
    for(unsigned i=0;i<3;++i) { s.feet[i]=f64(p+32+i*8); if(!std::isfinite(s.feet[i])) return false; }
    for(unsigned i=0;i<count;++i) {
        Pose::Bone bone{}; bone.visible=u32(p+64+i*68);
        if(bone.visible>1) return false;
        for(unsigned j=0;j<16;++j) {
            bone.matrix[j]=f32(p+68+i*68+j*4);
            if(!std::isfinite(bone.matrix[j])) return false;
        }
        // Poses are affine, not projection matrices or arbitrary homogeneous w.
        if(bone.matrix[3]!=0 || bone.matrix[7]!=0 || bone.matrix[11]!=0 || bone.matrix[15]!=1) return false;
        s.bones.push_back(bone);
    }
    out=std::move(s); return true;
}
inline bool matches(const Asset& a,const Pose& p,std::uint64_t generation) {
    return a.generation==generation && p.generation==generation && a.identity==p.asset && a.parts==p.bones.size();
}
}

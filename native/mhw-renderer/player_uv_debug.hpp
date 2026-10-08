#pragma once
#include <array>
#include <cstdint>
namespace crafterhunter::player {
// Debug skin net only. Face order: model -Y(top), +Y(bottom), -X, -Z(front), +X, +Z(back).
// Green encodes face (32,64,96,128,160,192); red encodes body part.
inline std::array<std::uint8_t,64*64*4> uvDebugSkin(bool slim) {
    std::array<std::uint8_t,64*64*4> pixels{};
    struct Net { int u,v,w,h,d,part; };
    const int arm=slim?3:4;
    const Net nets[]={{0,0,8,8,8,1},{32,0,8,8,8,1},
        {16,16,8,12,4,2},{16,32,8,12,4,2},
        {40,16,arm,12,4,3},{40,32,arm,12,4,3},
        {32,48,arm,12,4,4},{48,48,arm,12,4,4},
        {0,16,4,12,4,5},{0,32,4,12,4,5},
        {16,48,4,12,4,6},{0,48,4,12,4,6}};
    for(auto n:nets) {
        const int rects[][4]={{n.u+n.d,n.v,n.w,n.d},{n.u+n.d+n.w,n.v,n.w,n.d},
            {n.u,n.v+n.d,n.d,n.h},{n.u+n.d,n.v+n.d,n.w,n.h},
            {n.u+n.d+n.w,n.v+n.d,n.d,n.h},{n.u+2*n.d+n.w,n.v+n.d,n.w,n.h}};
        for(int f=0;f<6;++f) {
            auto r=rects[f];
            for(int y=0;y<r[3];++y) for(int x=0;x<r[2];++x) {
                auto i=((r[1]+y)*64+r[0]+x)*4;
                pixels[i]=n.part*32; pixels[i+1]=(f+1)*32;
                // Top-left white marker, blue increases down the face: exposes flips/rotation.
                pixels[i+2]=32+192*y/(r[3]-1); pixels[i+3]=255;
                if(x==0 && y==0) pixels[i]=pixels[i+1]=pixels[i+2]=255;
            }
        }
    }
    return pixels;
}
}

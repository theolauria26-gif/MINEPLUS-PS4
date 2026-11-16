#include "pcg_world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <climits>

namespace mineplus {
namespace {
inline float lerp(float a, float b, float t) { return a + (b-a)*t; }
inline float smooth(float t) { return t*t*(3.0f-2.0f*t); }
inline std::int32_t floor_i(float v) { return static_cast<std::int32_t>(std::floor(v)); }
}

std::uint32_t PcgWorld::hash(std::int32_t x, std::int32_t y, std::uint32_t salt) const {
    std::uint64_t n = seed_ ^ (std::uint64_t(std::uint32_t(x))*0x9E3779B185EBCA87ULL)
                            ^ (std::uint64_t(std::uint32_t(y))*0xC2B2AE3D27D4EB4FULL)
                            ^ (std::uint64_t(salt)*0x165667B19E3779F9ULL);
    n ^= n >> 30; n *= 0xBF58476D1CE4E5B9ULL;
    n ^= n >> 27; n *= 0x94D049BB133111EBULL;
    n ^= n >> 31;
    return static_cast<std::uint32_t>(n >> 16);
}

float PcgWorld::value_noise(float x, float y, std::uint32_t salt) const {
    const int ix=floor_i(x), iy=floor_i(y);
    const float fx=smooth(x-ix), fy=smooth(y-iy);
    auto v=[&](int a,int b){ return float(hash(a,b,salt)&0xFFFFu)/32767.5f-1.0f; };
    return lerp(lerp(v(ix,iy),v(ix+1,iy),fx),
                lerp(v(ix,iy+1),v(ix+1,iy+1),fx),fy);
}

float PcgWorld::fbm(float x,float y,std::uint32_t salt,int octaves,float persistence) const {
    float sum=0, amplitude=1, norm=0;
    for(int i=0;i<octaves;++i){
        sum += value_noise(x,y,salt+std::uint32_t(i)*1013u)*amplitude;
        norm += amplitude; amplitude *= persistence; x *= 2.03f; y *= 2.03f;
    }
    return norm>0 ? sum/norm : 0;
}

float PcgWorld::continent_mask(float x,float y) const {
    // Continente central + faixa oceânica; fora dela os macro-biomas continuam sem fim.
    const float radius=4200.0f;
    const float distance=std::sqrt(x*x+y*y);
    const float edge=std::max(0.0f,std::min(1.0f,(distance-radius)/900.0f));
    return 1.0f-edge;
}

Biome PcgWorld::choose_biome(float temp,float wet,float elevation,float continent) const {
    if(continent<0.18f && elevation<0.05f) return Biome::Ocean;
    if(elevation>0.66f) return Biome::Highlands;
    if(temp< -0.34f) return Biome::Tundra;
    if(temp>0.42f && wet< -0.12f) return Biome::Desert;
    if(temp>0.30f && wet>0.35f) return Biome::Volcanic;
    if(wet< -0.42f) return Biome::CrystalWastes;
    if(wet>0.05f) return Biome::Forest;
    return Biome::Plains;
}

SurfaceSample PcgWorld::sample_surface(std::int32_t x,std::int32_t y) const {
    const float fx=float(x), fy=float(y);
    const float broad=fbm(fx/1500.0f,fy/1500.0f,11,5,0.52f);
    const float ridge=1.0f-std::fabs(fbm(fx/410.0f,fy/410.0f,29,4,0.56f));
    const float detail=fbm(fx/75.0f,fy/75.0f,47,3,0.48f);
    const float continent=continent_mask(fx,fy);
    const float elevation=broad*0.34f+ridge*0.42f+detail*0.12f+continent*0.18f-0.32f;
    const float latitudeCooling=static_cast<float>(std::fabs(fy)/12000.0f);
    const float temperature=fbm(fx/2300.0f,fy/2300.0f,71,3,0.5f)-std::min(0.8f,latitudeCooling);
    const float moisture=fbm(fx/1250.0f,fy/1250.0f,97,4,0.53f);
    const Biome biome=choose_biome(temperature,moisture,elevation,continent);
    int height=static_cast<int>(34.0f+elevation*58.0f);
    std::uint8_t tile=1; // grass
    switch(biome){
        case Biome::Ocean: height=18+int((detail+1.0f)*3.0f); tile=13; break; // water
        case Biome::Plains: tile=1; break;
        case Biome::Forest: tile=5; break; // moss/forest floor
        case Biome::Desert: tile=9; height-=3; break;
        case Biome::Tundra: tile=10; break;
        case Biome::Highlands: tile=3; height+=12; break;
        case Biome::Volcanic: tile=16; height+=5; break;
        case Biome::CrystalWastes: tile=6; break;
    }
    height=std::max(1,std::min(WORLD_HEIGHT-1,height));
    return {static_cast<std::int16_t>(height),tile,
            static_cast<std::uint8_t>(biome),
            static_cast<std::uint8_t>(std::clamp(int((moisture+1.0f)*127.5f),0,255)),
            static_cast<std::uint8_t>(std::clamp(int((temperature+1.0f)*127.5f),0,255))};
}

bool PcgWorld::structure_anchor_for_cell(std::int32_t ax,std::int32_t ay,
                                                StructureAnchor& out) const {
    constexpr std::int32_t CELL=96;
    const auto h=hash(ax,ay,313);
    if(h%11u!=0u) return false;
    const std::int64_t sx=std::int64_t(ax)*CELL+std::int64_t(hash(ax,ay,317)%CELL);
    const std::int64_t sy=std::int64_t(ay)*CELL+std::int64_t(hash(ax,ay,331)%CELL);
    if(sx<INT32_MIN||sx>INT32_MAX||sy<INT32_MIN||sy>INT32_MAX) return false;
    const std::int32_t radius=7+std::int32_t(hash(ax,ay,337)%18);
    out={static_cast<std::int32_t>(sx),static_cast<std::int32_t>(sy),
         static_cast<std::uint16_t>(1+hash(ax,ay,347)%4),
         static_cast<std::uint16_t>(radius)};
    return true;
}

bool PcgWorld::structure_at(std::int32_t x,std::int32_t y,StructureAnchor& out) const {
    // Query neighboring macro-cells because a structure can cross chunk borders.
    constexpr int CELL=96;
    const int cx=x/CELL-((x%CELL)!=0&&x<0?1:0);
    const int cy=y/CELL-((y%CELL)!=0&&y<0?1:0);
    bool found=false; std::uint64_t best=~std::uint64_t(0);
    for(int oy=-1;oy<=1;++oy) for(int ox=-1;ox<=1;++ox){
        StructureAnchor candidate{};
        if(!structure_anchor_for_cell(cx+ox,cy+oy,candidate)) continue;
        const std::int64_t dx=std::int64_t(x)-candidate.x;
        const std::int64_t dy=std::int64_t(y)-candidate.y;
        if(dx*dx+dy*dy>std::int64_t(candidate.radius)*candidate.radius) continue;
        const std::uint64_t score=std::uint64_t(dx*dx+dy*dy);
        if(score<best){best=score;out=candidate;found=true;}
    }
    return found;
}
} // namespace mineplus

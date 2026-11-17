#include "surface_settlements.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace mineplus {
namespace {
std::uint32_t mix(std::int32_t x, std::int32_t y, std::uint32_t salt) {
    std::uint32_t n=static_cast<std::uint32_t>(x)*374761393u+
        static_cast<std::uint32_t>(y)*668265263u+salt;
    n=(n^(n>>13))*1274126177u;
    return n^(n>>16);
}
bool suitable(const SurfaceSample& s) {
    const auto biome=static_cast<Biome>(s.biome);
    return (biome==Biome::Plains || biome==Biome::Forest) &&
           s.surface!=static_cast<std::uint8_t>(VoxelMaterial::Water) &&
           s.surface!=static_cast<std::uint8_t>(VoxelMaterial::Sand) &&
           s.surface!=static_cast<std::uint8_t>(VoxelMaterial::Lava) &&
           s.moisture>=75u && s.temperature>=35u;
}
constexpr SettlementBuilding layouts[4][5] = {
    {{0,0,4,0},{-11,-5,3,1},{10,-7,3,2},{-9,9,3,3},{10,9,3,4}},
    {{0,0,4,0},{-10,-8,3,2},{10,-8,3,1},{-10,8,3,4},{10,8,3,3}},
    {{0,0,4,0},{-12,0,3,1},{0,-12,3,2},{12,0,3,3},{0,12,3,4}},
    {{0,0,4,0},{-12,-6,3,3},{12,-6,3,1},{-12,8,3,2},{12,8,3,4}}
};
}

bool surface_settlement_for_cell(const PcgWorld& world,
                                 std::int32_t cell_x,
                                 std::int32_t cell_y,
                                 SurfaceSettlement& out) {
    constexpr std::int32_t CELL=320;
    constexpr std::int32_t LIMIT=std::numeric_limits<std::int32_t>::max()/CELL-1;
    if(cell_x < -LIMIT || cell_x > LIMIT ||
       cell_y < -LIMIT || cell_y > LIMIT) return false;
    const std::uint32_t seed=mix(cell_x,cell_y,0x51E771Eu);
    if(seed%7u!=0u) return false;
    const std::int32_t x=cell_x*CELL+32+
        static_cast<std::int32_t>((seed>>8)%256u);
    const std::int32_t y=cell_y*CELL+32+
        static_cast<std::int32_t>((mix(cell_y,cell_x,0xA11CEu)>>8)%256u);
    const SurfaceSample center=world.sample_surface(x,y);
    if(!suitable(center)) return false;
    const std::int32_t offsets[4][2]={{-12,0},{12,0},{0,-12},{0,12}};
    for(const auto& offset:offsets) {
        const SurfaceSample nearby=world.sample_surface(x+offset[0],y+offset[1]);
        if(!suitable(nearby) || std::abs(int(nearby.height)-int(center.height))>3)
            return false;
    }
    out={x,y,static_cast<std::uint8_t>((seed>>20)%4u),
         static_cast<std::uint8_t>(5u+(seed%3u))};
    return true;
}

bool surface_settlement_building(const SurfaceSettlement& settlement,
                                 std::uint8_t index,
                                 SettlementBuilding& out) {
    if(index>=5u || settlement.layout>=4u) return false;
    out=layouts[settlement.layout][index];
    return true;
}

} // namespace mineplus

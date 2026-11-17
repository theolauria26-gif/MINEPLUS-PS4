#pragma once
#include "pcg_world.hpp"
#include <cstdint>

namespace mineplus {

struct SurfaceSettlement {
    std::int32_t x;
    std::int32_t y;
    std::uint8_t layout;
    std::uint8_t resident_count;
};

struct SettlementBuilding {
    std::int16_t offset_x;
    std::int16_t offset_y;
    std::uint8_t half_extent;
    std::uint8_t role;
};

bool surface_settlement_for_cell(const PcgWorld& world,
                                 std::int32_t cell_x,
                                 std::int32_t cell_y,
                                 SurfaceSettlement& out);
bool surface_settlement_building(const SurfaceSettlement& settlement,
                                 std::uint8_t index,
                                 SettlementBuilding& out);

} // namespace mineplus

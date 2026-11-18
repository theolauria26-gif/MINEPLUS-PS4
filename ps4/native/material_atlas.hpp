#pragma once
#include <cstdint>
namespace mineplus {
constexpr std::uint8_t MATERIAL_ATLAS_FIRST_ID = 1;
constexpr std::uint8_t MATERIAL_ATLAS_LAST_ID = 23;
constexpr int MATERIAL_ATLAS_TILE_SIZE = 32;
constexpr int MATERIAL_ATLAS_COLUMNS = 8;
constexpr int MATERIAL_ATLAS_ROWS = 4;
constexpr int MATERIAL_ATLAS_WIDTH = MATERIAL_ATLAS_TILE_SIZE * MATERIAL_ATLAS_COLUMNS;
constexpr int MATERIAL_ATLAS_HEIGHT = MATERIAL_ATLAS_TILE_SIZE * MATERIAL_ATLAS_ROWS;
struct MaterialAtlasRegion {
    float u_min = 0.0f, v_min = 0.0f, u_max = 0.0f, v_max = 0.0f;
    std::uint8_t tile_x = 0, tile_y = 0;
};
// Returns false for air, reserved cells, and unknown material IDs.
bool material_atlas_region(std::uint8_t material, MaterialAtlasRegion& output);
} // namespace mineplus

#include "material_atlas.hpp"
namespace mineplus {
bool material_atlas_region(std::uint8_t material, MaterialAtlasRegion& output) {
    output = MaterialAtlasRegion{};
    if (material < MATERIAL_ATLAS_FIRST_ID || material > MATERIAL_ATLAS_LAST_ID)
        return false;
    const unsigned int cell = static_cast<unsigned int>(material - MATERIAL_ATLAS_FIRST_ID);
    const unsigned int x = cell % MATERIAL_ATLAS_COLUMNS;
    const unsigned int y = cell / MATERIAL_ATLAS_COLUMNS;
    constexpr float half_texel = 0.5f;
    constexpr float atlas_width = static_cast<float>(MATERIAL_ATLAS_WIDTH);
    constexpr float atlas_height = static_cast<float>(MATERIAL_ATLAS_HEIGHT);
    output.tile_x = static_cast<std::uint8_t>(x);
    output.tile_y = static_cast<std::uint8_t>(y);
    output.u_min = (x * MATERIAL_ATLAS_TILE_SIZE + half_texel) / atlas_width;
    output.v_min = (y * MATERIAL_ATLAS_TILE_SIZE + half_texel) / atlas_height;
    output.u_max = ((x + 1) * MATERIAL_ATLAS_TILE_SIZE - half_texel) / atlas_width;
    output.v_max = ((y + 1) * MATERIAL_ATLAS_TILE_SIZE - half_texel) / atlas_height;
    return true;
}
} // namespace mineplus

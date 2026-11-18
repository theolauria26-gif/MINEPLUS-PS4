#include "material_atlas.hpp"
#include <cmath>
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

bool material_atlas_resolve_uv(std::uint8_t material, float tile_u, float tile_v,
                               float& atlas_u, float& atlas_v) {
    atlas_u = 0.0f;
    atlas_v = 0.0f;
    MaterialAtlasRegion region{};
    if (!material_atlas_region(material, region) ||
        !std::isfinite(tile_u) || !std::isfinite(tile_v)) return false;
    const float wrapped_u = tile_u - std::floor(tile_u);
    const float wrapped_v = tile_v - std::floor(tile_v);
    atlas_u = region.u_min + wrapped_u * (region.u_max - region.u_min);
    atlas_v = region.v_min + wrapped_v * (region.v_max - region.v_min);
    return true;
}
} // namespace mineplus

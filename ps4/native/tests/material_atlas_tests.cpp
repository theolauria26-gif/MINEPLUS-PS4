#include "material_atlas.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
using namespace mineplus;
int main() {
    MaterialAtlasRegion region{};
    assert(!material_atlas_region(0, region));
    assert(region.u_min == 0.0f && region.v_min == 0.0f);
    assert(material_atlas_region(22, region));
    assert(region.tile_x == 5 && region.tile_y == 2);
    assert(material_atlas_region(23, region));
    assert(region.tile_x == 6 && region.tile_y == 2);
    assert(!material_atlas_region(255, region));
    bool occupied[MATERIAL_ATLAS_ROWS][MATERIAL_ATLAS_COLUMNS] = {};
    for (std::uint8_t id = MATERIAL_ATLAS_FIRST_ID; id <= MATERIAL_ATLAS_LAST_ID; ++id) {
        assert(material_atlas_region(id, region));
        assert(region.tile_x < MATERIAL_ATLAS_COLUMNS && region.tile_y < MATERIAL_ATLAS_ROWS);
        assert(!occupied[region.tile_y][region.tile_x]);
        occupied[region.tile_y][region.tile_x] = true;
        const unsigned int cell = id - MATERIAL_ATLAS_FIRST_ID;
        assert(region.tile_x == cell % MATERIAL_ATLAS_COLUMNS);
        assert(region.tile_y == cell / MATERIAL_ATLAS_COLUMNS);
        const float u0 = (region.tile_x * MATERIAL_ATLAS_TILE_SIZE + 0.5f) / MATERIAL_ATLAS_WIDTH;
        const float v0 = (region.tile_y * MATERIAL_ATLAS_TILE_SIZE + 0.5f) / MATERIAL_ATLAS_HEIGHT;
        const float u1 = ((region.tile_x + 1) * MATERIAL_ATLAS_TILE_SIZE - 0.5f) / MATERIAL_ATLAS_WIDTH;
        const float v1 = ((region.tile_y + 1) * MATERIAL_ATLAS_TILE_SIZE - 0.5f) / MATERIAL_ATLAS_HEIGHT;
        assert(std::fabs(region.u_min - u0) < 0.000001f);
        assert(std::fabs(region.v_min - v0) < 0.000001f);
        assert(std::fabs(region.u_max - u1) < 0.000001f);
        assert(std::fabs(region.v_max - v1) < 0.000001f);
        assert(region.u_min < region.u_max && region.v_min < region.v_max);
        assert(region.u_min > 0.0f && region.u_max < 1.0f);
        assert(region.v_min > 0.0f && region.v_max < 1.0f);
    }
    assert(material_atlas_region(MATERIAL_ATLAS_LAST_ID, region));
    assert(region.tile_x == 6 && region.tile_y == 2);
    std::cout << "MINEPLUS material atlas tests: OK\n";

    float atlas_u = -1.0f, atlas_v = -1.0f;
    assert(material_atlas_resolve_uv(1, 0.0f, 0.0f, atlas_u, atlas_v));
    MaterialAtlasRegion grass{};
    assert(material_atlas_region(1, grass));
    assert(std::fabs(atlas_u - grass.u_min) < 0.000001f);
    assert(std::fabs(atlas_v - grass.v_min) < 0.000001f);
    assert(material_atlas_resolve_uv(1, 1.0f, 2.0f, atlas_u, atlas_v));
    assert(std::fabs(atlas_u - grass.u_min) < 0.000001f);
    assert(std::fabs(atlas_v - grass.v_min) < 0.000001f);
    assert(material_atlas_resolve_uv(1, 2.25f, -0.25f, atlas_u, atlas_v));
    assert(std::fabs(atlas_u - (grass.u_min + 0.25f * (grass.u_max - grass.u_min))) < 0.000001f);
    assert(std::fabs(atlas_v - (grass.v_min + 0.75f * (grass.v_max - grass.v_min))) < 0.000001f);
    assert(!material_atlas_resolve_uv(0, 0.5f, 0.5f, atlas_u, atlas_v));
    assert(atlas_u == 0.0f && atlas_v == 0.0f);
    assert(!material_atlas_resolve_uv(1, NAN, 0.5f, atlas_u, atlas_v));
    assert(atlas_u == 0.0f && atlas_v == 0.0f);
    return 0;
}

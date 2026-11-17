#pragma once
#include "material_atlas.hpp"
#include <cstdint>
#include <vector>
namespace mineplus {
// Builds a deterministic, original pixel-art atlas in row-major RGBA8.
// Unassigned atlas cells remain transparent; all defined material pixels opaque.
bool build_procedural_material_atlas(std::vector<std::uint8_t>& rgba);
} // namespace mineplus

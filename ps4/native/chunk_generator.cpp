#include "chunk_generator.hpp"
#include <algorithm>
#include <climits>
#include <cstddef>

namespace mineplus {

std::int32_t ChunkGenerator::floor_div(std::int32_t value, std::int32_t divisor) {
    // C++ trunca divisão negativa em direção a zero; chunks precisam de floor.
    const std::int32_t q = value / divisor;
    const std::int32_t r = value % divisor;
    return (r != 0 && ((r < 0) != (divisor < 0))) ? q - 1 : q;
}

void ChunkGenerator::generate(std::int32_t cx, std::int32_t cy, ChunkData& out) const {
    out.chunk_x = cx;
    out.chunk_y = cy;
    out.seed = world_.seed();

    constexpr std::int32_t S = PcgWorld::CHUNK_SIZE;
    const std::int64_t origin_x = std::int64_t(cx) * S;
    const std::int64_t origin_y = std::int64_t(cy) * S;

    for (std::int32_t ly = 0; ly < S; ++ly) {
        for (std::int32_t lx = 0; lx < S; ++lx) {
            const auto index = static_cast<std::size_t>(ly * S + lx);
            const auto wx = static_cast<std::int32_t>(std::clamp<std::int64_t>(
                origin_x + lx, INT32_MIN, INT32_MAX));
            const auto wy = static_cast<std::int32_t>(std::clamp<std::int64_t>(
                origin_y + ly, INT32_MIN, INT32_MAX));
            out.cells[index] = world_.sample_surface(wx, wy);
            StructureAnchor anchor{};
            if (world_.structure_at(wx, wy, anchor)) {
                out.structures[index] = anchor;
                out.has_structure[index] = 1;
            } else {
                out.structures[index] = {};
                out.has_structure[index] = 0;
            }
        }
    }
}

VoxelSample ChunkGenerator::sample_voxel(std::int32_t x, std::int32_t y,
                                          std::int32_t z) const {
    return world_.sample_voxel(x, y, z);
}

} // namespace mineplus

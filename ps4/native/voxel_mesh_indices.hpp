#pragma once
#include "voxel_mesher.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace mineplus {
// Emits two outward-facing triangles (six uint32 indices) for each quad.
// Vertices are expected in the builder's 0,1,2,3 outward-CCW order.
class VoxelMeshIndexBuilder {
public:
    static bool build(const std::vector<MeshQuad>& quads,
                      std::size_t vertex_count,
                      std::vector<std::uint32_t>& output,
                      std::size_t max_indices = 393216);
};
} // namespace mineplus

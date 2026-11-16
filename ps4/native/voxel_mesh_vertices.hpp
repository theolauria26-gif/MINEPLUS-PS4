#pragma once
#include "voxel_mesher.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace mineplus {
// CPU-side vertex contract for a future PS4 GPU renderer. UVs are tile-local
// voxel units (not atlas coordinates): the shader must repeat within the
// material cell, then map through material_atlas_region().
struct VoxelMeshVertex {
    float x=0, y=0, z=0;
    float u=0, v=0;
    std::uint8_t material=0;
    std::int8_t normal_x=0, normal_y=0, normal_z=0;
};
class VoxelMeshVertexBuilder {
public:
    // Emits four vertices per greedy quad in outward CCW order. On invalid
    // input or budget overflow, clears output and returns false.
    static bool build(const std::vector<MeshQuad>& quads,
                      std::vector<VoxelMeshVertex>& output,
                      std::size_t max_vertices=262144);
};
} // namespace mineplus

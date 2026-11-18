#pragma once
#include "voxel_mesh_vertices.hpp"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace mineplus {

// Stable CPU staging format for a future GPU vertex buffer.
// Position/UV/normal are float attributes; material is a uint32 shader attribute.
// This is deliberately independent of any PS4 GNM descriptor or shader ABI.
struct VoxelGpuVertex {
    float x = 0, y = 0, z = 0;
    float u = 0, v = 0;
    float nx = 0, ny = 0, nz = 0;
    std::uint32_t material = 0;
};

static_assert(std::is_standard_layout<VoxelGpuVertex>::value,
              "VoxelGpuVertex must remain standard-layout");
static_assert(std::is_trivially_copyable<VoxelGpuVertex>::value,
              "VoxelGpuVertex must be safe to copy into a staging buffer");
static_assert(sizeof(VoxelGpuVertex) == 36,
              "VoxelGpuVertex staging stride changed");
static_assert(offsetof(VoxelGpuVertex, x) == 0, "position offset changed");
static_assert(offsetof(VoxelGpuVertex, u) == 12, "UV offset changed");
static_assert(offsetof(VoxelGpuVertex, nx) == 20, "normal offset changed");
static_assert(offsetof(VoxelGpuVertex, material) == 32,
              "material offset changed");

// Converts validated mesh vertices into a deterministic, tightly-defined
// staging layout. This does not allocate GPU memory or issue GPU commands.
class VoxelGpuUploadBuilder {
public:
    static bool build(const std::vector<VoxelMeshVertex>& source,
                      std::vector<VoxelGpuVertex>& output,
                      std::size_t max_vertices = 262144);
};

// Validates a complete CPU mesh package and prepares matching vertex/index streams.
// This remains CPU staging; the PS4 backend must still allocate and upload buffers.
class VoxelGpuMeshUploadBuilder {
public:
    static bool build(const std::vector<VoxelMeshVertex>& source_vertices,
                      const std::vector<std::uint32_t>& source_indices,
                      std::vector<VoxelGpuVertex>& output_vertices,
                      std::vector<std::uint32_t>& output_indices,
                      std::size_t max_vertices = 262144,
                      std::size_t max_indices = 393216);
};

} // namespace mineplus

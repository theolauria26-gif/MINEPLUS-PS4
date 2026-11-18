#pragma once
#include "pcg_world.hpp"
#include "voxel_mesher.hpp"
#include "voxel_mesh_vertices.hpp"
#include "world_runtime.hpp"
#include <cstdint>

namespace mineplus {
struct ProceduralChunkOrigin { std::int32_t x = 0; std::int32_t y = 0; };
// Materializes solid terrain into a chunk and its one-voxel horizontal halo.
// Liquids are omitted until a separate translucent render pass exists.
class ProceduralVoxelChunkBuilder {
public:
 static bool build(const PcgWorld& world, ProceduralChunkOrigin origin, VoxelChunk& output);
 // Runtime overload also applies player voxel and surface edits.
 static bool build(WorldRuntime& world, ProceduralChunkOrigin origin, VoxelChunk& output);
};

// Complete CPU-side path from procedural voxels to GPU-contract vertices.
// This does not upload or draw anything; it produces validated, renderer-ready data.
struct ProceduralVoxelMesh {
 std::vector<MeshQuad> quads;
 std::vector<VoxelMeshVertex> vertices;
};
class ProceduralVoxelMeshBuilder {
public:
 static bool build(const PcgWorld& world, ProceduralChunkOrigin origin,
                   ProceduralVoxelMesh& output,
                   std::size_t max_quads = 65536,
                   std::size_t max_vertices = 262144);
 static bool build(WorldRuntime& world, ProceduralChunkOrigin origin,
                   ProceduralVoxelMesh& output,
                   std::size_t max_quads = 65536,
                   std::size_t max_vertices = 262144);
};
} // namespace mineplus

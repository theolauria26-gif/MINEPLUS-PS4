#include "procedural_voxel_chunk.hpp"
#include "voxel_mesh_vertices.hpp"
#include <limits>
namespace mineplus {
bool ProceduralVoxelChunkBuilder::build(const PcgWorld& world, ProceduralChunkOrigin origin, VoxelChunk& output) {
 const std::int64_t min_x=std::int64_t(origin.x)-1, min_y=std::int64_t(origin.y)-1;
 const std::int64_t max_x=std::int64_t(origin.x)+VoxelChunk::SIZE_X, max_y=std::int64_t(origin.y)+VoxelChunk::SIZE_Y;
 if(min_x<std::numeric_limits<std::int32_t>::min() || min_y<std::numeric_limits<std::int32_t>::min() || max_x>std::numeric_limits<std::int32_t>::max() || max_y>std::numeric_limits<std::int32_t>::max()) return false;
 VoxelChunk generated{};
 for(int z=-1;z<=VoxelChunk::SIZE_Z;++z) for(int y=-1;y<=VoxelChunk::SIZE_Y;++y) for(int x=-1;x<=VoxelChunk::SIZE_X;++x) {
  const auto wx=static_cast<std::int32_t>(std::int64_t(origin.x)+x), wy=static_cast<std::int32_t>(std::int64_t(origin.y)+y);
  const VoxelSample s=world.sample_voxel(wx,wy,z);
  if(!s.solid || s.liquid) continue;
  const auto material=static_cast<std::uint8_t>(s.material);
  if(material==0 || !generated.set(x,y,z,material)) return false;
 }
 output=generated; return true;
}

bool ProceduralVoxelChunkBuilder::build(WorldRuntime& world, ProceduralChunkOrigin origin, VoxelChunk& output) {
 const std::int64_t min_x=std::int64_t(origin.x)-1, min_y=std::int64_t(origin.y)-1;
 const std::int64_t max_x=std::int64_t(origin.x)+VoxelChunk::SIZE_X, max_y=std::int64_t(origin.y)+VoxelChunk::SIZE_Y;
 if(min_x<std::numeric_limits<std::int32_t>::min() || min_y<std::numeric_limits<std::int32_t>::min() || max_x>std::numeric_limits<std::int32_t>::max() || max_y>std::numeric_limits<std::int32_t>::max()) return false;
 world.update_center(origin.x + VoxelChunk::SIZE_X / 2, origin.y + VoxelChunk::SIZE_Y / 2, 1);
 VoxelChunk generated{};
 for(int z=0;z<VoxelChunk::SIZE_Z;++z) for(int y=-1;y<=VoxelChunk::SIZE_Y;++y) for(int x=-1;x<=VoxelChunk::SIZE_X;++x) {
  const auto wx=static_cast<std::int32_t>(std::int64_t(origin.x)+x), wy=static_cast<std::int32_t>(std::int64_t(origin.y)+y);
  VoxelSample sample{};
  if(!world.get_voxel(wx,wy,z,sample)) return false;
  if(!sample.solid || sample.liquid) continue;
  const auto material=static_cast<std::uint8_t>(sample.material);
  if(material==0 || !generated.set(x,y,z,material)) return false;
 }
 output=generated; return true;
}

bool ProceduralVoxelMeshBuilder::build(const PcgWorld& world,
                                       ProceduralChunkOrigin origin,
                                       ProceduralVoxelMesh& output,
                                       std::size_t max_quads,
                                       std::size_t max_vertices) {
 VoxelChunk voxels{};
 if(!ProceduralVoxelChunkBuilder::build(world,origin,voxels)) return false;
 ProceduralVoxelMesh generated;
 if(!VoxelMesher::build(voxels,generated.quads,max_quads)) return false;
 if(!VoxelMeshVertexBuilder::build(generated.quads,generated.vertices,max_vertices)) return false;
 if(!VoxelMeshIndexBuilder::build(generated.quads,generated.vertices.size(),generated.indices)) return false;
 output.quads.swap(generated.quads);
 output.vertices.swap(generated.vertices);
 output.indices.swap(generated.indices);
 return true;
}

bool ProceduralVoxelMeshBuilder::build(WorldRuntime& world,
                                       ProceduralChunkOrigin origin,
                                       ProceduralVoxelMesh& output,
                                       std::size_t max_quads,
                                       std::size_t max_vertices) {
 VoxelChunk voxels{};
 if(!ProceduralVoxelChunkBuilder::build(world,origin,voxels)) return false;
 ProceduralVoxelMesh generated;
 if(!VoxelMesher::build(voxels,generated.quads,max_quads)) return false;
 if(!VoxelMeshVertexBuilder::build(generated.quads,generated.vertices,max_vertices)) return false;
 output.quads.swap(generated.quads);
 output.vertices.swap(generated.vertices);
 return true;
}

} // namespace mineplus

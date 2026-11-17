#include "procedural_voxel_chunk.hpp"
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
} // namespace mineplus

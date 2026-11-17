#include "procedural_voxel_chunk.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <climits>
using namespace mineplus;
int main(){
 PcgWorld world(0x4D504C5553ULL); VoxelChunk a,repeat,east,negative;
 assert(ProceduralVoxelChunkBuilder::build(world,{0,0},a));
 assert(ProceduralVoxelChunkBuilder::build(world,{0,0},repeat)); assert(a.cells==repeat.cells);
 for(int z=0;z<VoxelChunk::SIZE_Z;++z) for(int y=0;y<VoxelChunk::SIZE_Y;++y) for(int x=0;x<VoxelChunk::SIZE_X;++x){
  const auto s=world.sample_voxel(x,y,z); const auto expected=(s.solid&&!s.liquid)?static_cast<std::uint8_t>(s.material):0;
  assert(a.get(x,y,z)==expected);
 }
 assert(ProceduralVoxelChunkBuilder::build(world,{16,0},east));
 for(int z=0;z<VoxelChunk::SIZE_Z;++z) for(int y=0;y<VoxelChunk::SIZE_Y;++y){assert(a.get(16,y,z)==east.get(0,y,z)); assert(a.get(15,y,z)==east.get(-1,y,z));}
 assert(ProceduralVoxelChunkBuilder::build(world,{-16,-16},negative));
 assert(negative.get(-1,-1,128)==0);
 assert(!ProceduralVoxelChunkBuilder::build(world,{INT32_MAX,0},negative));
 assert(!ProceduralVoxelChunkBuilder::build(world,{0,INT32_MIN},negative));
 WorldRuntime runtime(0x4D504C5553ULL);
 assert(runtime.set_voxel(4,5,40,VoxelMaterial::Crystal));
 VoxelChunk edited;
 assert(ProceduralVoxelChunkBuilder::build(runtime,{0,0},edited));
 assert(edited.get(4,5,40)==static_cast<std::uint8_t>(VoxelMaterial::Crystal));
 assert(runtime.set_voxel(4,5,40,VoxelMaterial::Air));
 assert(ProceduralVoxelChunkBuilder::build(runtime,{0,0},edited));
 assert(edited.get(4,5,40)==0);
 const auto preserved=edited.cells;
 assert(!ProceduralVoxelChunkBuilder::build(runtime,{INT32_MAX,0},edited));
 assert(edited.cells==preserved);
 std::cout<<"MINEPLUS procedural voxel chunk tests: OK\n";
}

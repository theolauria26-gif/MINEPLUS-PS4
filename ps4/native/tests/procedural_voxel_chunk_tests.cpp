#include "procedural_voxel_chunk.hpp"
#include "voxel_gpu_upload.hpp"
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
 ProceduralVoxelMesh mesh;
 assert(ProceduralVoxelMeshBuilder::build(world,{0,0},mesh));
 assert(!mesh.quads.empty());
 assert(mesh.vertices.size()==mesh.quads.size()*4);
 assert(mesh.indices.size()==mesh.quads.size()*6);
 std::vector<VoxelGpuVertex> staging;
 std::vector<std::uint32_t> staged_indices;
 assert(VoxelGpuMeshUploadBuilder::build(mesh.vertices,mesh.indices,staging,staged_indices));
 assert(staged_indices==mesh.indices);
 assert(staging.size()==mesh.vertices.size());
 for(std::size_t i=0;i<staging.size();++i) {
  assert(staging[i].x==mesh.vertices[i].x && staging[i].y==mesh.vertices[i].y && staging[i].z==mesh.vertices[i].z);
  assert(staging[i].u==mesh.vertices[i].u && staging[i].v==mesh.vertices[i].v);
  assert(staging[i].material==mesh.vertices[i].material);
 }
 for(std::size_t q=0;q<mesh.quads.size();++q) {
  const std::uint32_t base=static_cast<std::uint32_t>(q*4);
  const std::size_t at=q*6;
  assert(mesh.indices[at]==base && mesh.indices[at+1]==base+1 && mesh.indices[at+2]==base+2);
  assert(mesh.indices[at+3]==base && mesh.indices[at+4]==base+2 && mesh.indices[at+5]==base+3);
  for(std::size_t i=0;i<6;++i) assert(mesh.indices[at+i]<mesh.vertices.size());
 }
 std::vector<std::uint32_t> rejected_indices{99u};
 assert(!VoxelMeshIndexBuilder::build(mesh.quads,mesh.vertices.size()-1,rejected_indices));
 assert(rejected_indices.empty());
 rejected_indices.push_back(99u);
 assert(!VoxelMeshIndexBuilder::build(mesh.quads,mesh.vertices.size(),rejected_indices,mesh.indices.size()-1));
 assert(rejected_indices.empty());
 std::vector<MeshQuad> no_quads;
 assert(VoxelMeshIndexBuilder::build(no_quads,0,rejected_indices));
 assert(rejected_indices.empty());
 auto triangle_normal_dot=[](const VoxelMeshVertex& a,const VoxelMeshVertex& b,const VoxelMeshVertex& c) {
  const float abx=b.x-a.x, aby=b.y-a.y, abz=b.z-a.z;
  const float acx=c.x-a.x, acy=c.y-a.y, acz=c.z-a.z;
  const float nx=aby*acz-abz*acy;
  const float ny=abz*acx-abx*acz;
  const float nz=abx*acy-aby*acx;
  return nx*a.normal_x+ny*a.normal_y+nz*a.normal_z;
 };
 for(std::size_t q=0;q<mesh.quads.size();++q) {
  const std::size_t base=q*4;
  const auto& v0=mesh.vertices[base];
  const auto& v1=mesh.vertices[base+1];
  const auto& v2=mesh.vertices[base+2];
  const auto& v3=mesh.vertices[base+3];
  assert(triangle_normal_dot(v0,v1,v2)>0.0f);
  assert(triangle_normal_dot(v0,v2,v3)>0.0f);
 }
 for(const auto& vertex:mesh.vertices) {
  assert(vertex.material!=0);
  const int normal_count=(vertex.normal_x!=0)+(vertex.normal_y!=0)+(vertex.normal_z!=0);
  assert(normal_count==1);
 }
 const auto old_vertex_count=mesh.vertices.size();
 assert(!ProceduralVoxelMeshBuilder::build(world,{0,0},mesh,1,262144));
 assert(mesh.vertices.size()==old_vertex_count);
 ProceduralVoxelMesh edited_mesh;
 assert(ProceduralVoxelMeshBuilder::build(runtime,{0,0},edited_mesh));
 assert(!edited_mesh.vertices.empty());
 std::cout<<"MINEPLUS procedural voxel chunk tests: OK\n";
}

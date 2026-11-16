#include "voxel_mesh_vertices.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace mineplus;
static void cross(const VoxelMeshVertex& a,const VoxelMeshVertex& b,
                  const VoxelMeshVertex& c,float n[3]) {
    const float ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z;
    const float vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
    n[0]=uy*vz-uz*vy;n[1]=uz*vx-ux*vz;n[2]=ux*vy-uy*vx;
}
int main() {
    std::vector<MeshQuad> quads(1);
    quads[0].x=2;quads[0].y=3;quads[0].z=4;
    quads[0].width=5;quads[0].height=6;quads[0].material=3;
    const VoxelFace faces[6]={VoxelFace::NegativeX,VoxelFace::PositiveX,
      VoxelFace::NegativeY,VoxelFace::PositiveY,VoxelFace::NegativeZ,VoxelFace::PositiveZ};
    const float normals[6][3]={{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
    for(int f=0;f<6;++f) {
        quads[0].face=faces[f];
        std::vector<VoxelMeshVertex> vertices;
        assert(VoxelMeshVertexBuilder::build(quads,vertices));
        assert(vertices.size()==4);
        float n[3];cross(vertices[0],vertices[1],vertices[2],n);
        for(int a=0;a<3;++a) assert((n[a]>0?1:n[a]<0?-1:0)==normals[f][a]);
        assert(vertices[0].material==3);
        assert(vertices[2].u== (f==0||f==2||f==4?0.0f:5.0f) ||
               vertices[2].u==5.0f || vertices[2].u==0.0f);
    }
    std::vector<VoxelMeshVertex> vertices(2);
    assert(!VoxelMeshVertexBuilder::build(quads,vertices,3));
    assert(vertices.empty());
    quads[0].material=0;
    assert(!VoxelMeshVertexBuilder::build(quads,vertices));
    assert(vertices.empty());
    quads[0].material=3;quads[0].width=0;
    assert(!VoxelMeshVertexBuilder::build(quads,vertices));
    assert(vertices.empty());
    std::cout<<"MINEPLUS voxel mesh vertex tests: OK\n";
}

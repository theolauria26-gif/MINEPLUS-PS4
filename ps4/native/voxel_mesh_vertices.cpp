#include "voxel_mesh_vertices.hpp"
#include "material_atlas.hpp"
#include "voxel_face_uv.hpp"
#include <limits>
namespace mineplus {
namespace {
void set_axis(float& x,float& y,float& z,unsigned axis,float value) {
    if(axis==0) x=value; else if(axis==1) y=value; else z=value;
}
void set_normal(VoxelMeshVertex& v,VoxelFace face) {
    switch(face) {
        case VoxelFace::NegativeX: v.normal_x=-1; break;
        case VoxelFace::PositiveX: v.normal_x=1; break;
        case VoxelFace::NegativeY: v.normal_y=-1; break;
        case VoxelFace::PositiveY: v.normal_y=1; break;
        case VoxelFace::NegativeZ: v.normal_z=-1; break;
        case VoxelFace::PositiveZ: v.normal_z=1; break;
    }
}
}
bool VoxelMeshVertexBuilder::build(const std::vector<MeshQuad>& quads,
    std::vector<VoxelMeshVertex>& output,std::size_t max_vertices) {
    output.clear();
    if(max_vertices<4 || quads.size()>max_vertices/4) return false;
    output.reserve(quads.size()*4);
    for(const MeshQuad& q:quads) {
        MaterialAtlasRegion region{};
        VoxelFaceUvBasis basis{};
        if(!material_atlas_region(q.material,region) ||
           !voxel_face_uv_basis(q.face,basis) || q.width==0 || q.height==0) {
            output.clear(); return false;
        }
        float base[3]={static_cast<float>(q.x),static_cast<float>(q.y),static_cast<float>(q.z)};
        const float u0=0.0f, u1=static_cast<float>(q.width);
        const float v0=0.0f, v1=static_cast<float>(q.height);
        const float us[4]={u0,u1,u1,u0};
        const float vs[4]={v0,v0,v1,v1};
        for(int i=0;i<4;++i) {
            VoxelMeshVertex vertex{};
            vertex.x=base[0]; vertex.y=base[1]; vertex.z=base[2];
            set_axis(vertex.x,vertex.y,vertex.z,basis.u_axis,
                     base[basis.u_axis]+(basis.u_sign>0?us[i]:static_cast<float>(q.width)-us[i]));
            set_axis(vertex.x,vertex.y,vertex.z,basis.v_axis,
                     base[basis.v_axis]+(basis.v_sign>0?vs[i]:static_cast<float>(q.height)-vs[i]));
            // The plane coordinate already lies on the exposed face.
            vertex.u=us[i]; vertex.v=vs[i]; vertex.material=q.material;
            set_normal(vertex,q.face);
            output.push_back(vertex);
        }
    }
    return true;
}
} // namespace mineplus

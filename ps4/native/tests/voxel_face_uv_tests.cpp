#include "voxel_face_uv.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace mineplus;
static int component(const VoxelFaceUvBasis& b, int axis, bool u) {
    const int selected = u ? b.u_axis : b.v_axis;
    if (selected != axis) return 0;
    return u ? b.u_sign : b.v_sign;
}
static void cross(const VoxelFaceUvBasis& b, int out[3]) {
    const int u[3] = {component(b,0,true),component(b,1,true),component(b,2,true)};
    const int v[3] = {component(b,0,false),component(b,1,false),component(b,2,false)};
    out[0]=u[1]*v[2]-u[2]*v[1];
    out[1]=u[2]*v[0]-u[0]*v[2];
    out[2]=u[0]*v[1]-u[1]*v[0];
}
int main() {
    const VoxelFace faces[6] = {VoxelFace::NegativeX,VoxelFace::PositiveX,
        VoxelFace::NegativeY,VoxelFace::PositiveY,VoxelFace::NegativeZ,VoxelFace::PositiveZ};
    const int normals[6][3] = {{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
    for (int i=0;i<6;++i) {
        VoxelFaceUvBasis basis{};
        assert(voxel_face_uv_basis(faces[i],basis));
        assert(basis.u_axis < 3 && basis.v_axis < 3);
        assert(basis.u_axis != basis.v_axis);
        assert((basis.u_sign == -1 || basis.u_sign == 1));
        assert((basis.v_sign == -1 || basis.v_sign == 1));
        int normal[3] = {};
        cross(basis,normal);
        for (int axis=0;axis<3;++axis) assert(normal[axis] == normals[i][axis]);
    }
    VoxelFaceUvBasis invalid{9,9,-1,-1};
    assert(!voxel_face_uv_basis(static_cast<VoxelFace>(255),invalid));
    assert(invalid.u_axis == 0 && invalid.v_axis == 0);
    assert(invalid.u_sign == 1 && invalid.v_sign == 1);
    std::cout << "MINEPLUS voxel face UV tests: OK\n";
    return 0;
}

#pragma once
#include "voxel_mesher.hpp"
#include <cstdint>
namespace mineplus {
// Describes how a face's texture axes map onto chunk world axes.
// Signs preserve a consistent outward-facing vertex winding.
struct VoxelFaceUvBasis {
    std::uint8_t u_axis = 0;
    std::uint8_t v_axis = 0;
    std::int8_t u_sign = 1;
    std::int8_t v_sign = 1;
};
// Axis values are X=0, Y=1, Z=2. Invalid face values return false.
bool voxel_face_uv_basis(VoxelFace face, VoxelFaceUvBasis& output);
} // namespace mineplus

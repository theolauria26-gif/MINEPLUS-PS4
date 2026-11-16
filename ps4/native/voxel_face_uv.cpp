#include "voxel_face_uv.hpp"
namespace mineplus {
bool voxel_face_uv_basis(VoxelFace face, VoxelFaceUvBasis& output) {
    output = VoxelFaceUvBasis{};
    switch (face) {
        case VoxelFace::NegativeX: output = {1, 2, -1, 1}; return true;
        case VoxelFace::PositiveX: output = {1, 2,  1, 1}; return true;
        case VoxelFace::NegativeY: output = {2, 0, -1, 1}; return true;
        case VoxelFace::PositiveY: output = {2, 0,  1, 1}; return true;
        case VoxelFace::NegativeZ: output = {0, 1, -1, 1}; return true;
        case VoxelFace::PositiveZ: output = {0, 1,  1, 1}; return true;
    }
    return false;
}
} // namespace mineplus

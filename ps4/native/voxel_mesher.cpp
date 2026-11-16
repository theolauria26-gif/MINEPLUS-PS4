#include "voxel_mesher.hpp"

#include <algorithm>
#include <array>

namespace mineplus {

std::size_t VoxelChunk::index(int x, int y, int z) {
    return (static_cast<std::size_t>(z + 1) * PAD_Y +
            static_cast<std::size_t>(y + 1)) * PAD_X +
           static_cast<std::size_t>(x + 1);
}

bool VoxelChunk::set(int x, int y, int z, std::uint8_t material) {
    if (x < -1 || x > SIZE_X || y < -1 || y > SIZE_Y ||
        z < -1 || z > SIZE_Z) return false;
    cells[index(x, y, z)] = material;
    return true;
}

std::uint8_t VoxelChunk::get(int x, int y, int z) const {
    if (x < -1 || x > SIZE_X || y < -1 || y > SIZE_Y ||
        z < -1 || z > SIZE_Z) return 0;
    return cells[index(x, y, z)];
}

namespace {
struct Axis {
    int normal;
    int u;
    int v;
};

constexpr Axis AXES[3] = {
    {0, 1, 2}, {1, 0, 2}, {2, 0, 1}
};

int dimension(int axis) {
    return axis == 0 ? VoxelChunk::SIZE_X :
           axis == 1 ? VoxelChunk::SIZE_Y : VoxelChunk::SIZE_Z;
}

void set_component(int& x, int& y, int& z, int axis, int value) {
    if (axis == 0) x = value;
    else if (axis == 1) y = value;
    else z = value;
}

VoxelFace face_for(int axis, int sign) {
    if (axis == 0) return sign < 0 ? VoxelFace::NegativeX : VoxelFace::PositiveX;
    if (axis == 1) return sign < 0 ? VoxelFace::NegativeY : VoxelFace::PositiveY;
    return sign < 0 ? VoxelFace::NegativeZ : VoxelFace::PositiveZ;
}
} // namespace

void VoxelMesher::build(const VoxelChunk& chunk, std::vector<MeshQuad>& output) {
    output.clear();

    // Each axis and direction gets a 2D material mask for a single voxel
    // plane. Greedy rectangle extraction turns the mask into compact quads.
    for (int axis = 0; axis < 3; ++axis) {
        const Axis basis = AXES[axis];
        const int uSize = dimension(basis.u);
        const int vSize = dimension(basis.v);
        std::vector<std::uint8_t> mask(
            static_cast<std::size_t>(uSize) * static_cast<std::size_t>(vSize));

        for (int sign = -1; sign <= 1; sign += 2) {
            const VoxelFace face = face_for(axis, sign);
            for (int plane = 0; plane < dimension(axis); ++plane) {
                std::fill(mask.begin(), mask.end(), 0);

                for (int v = 0; v < vSize; ++v) {
                    for (int u = 0; u < uSize; ++u) {
                        int x = 0, y = 0, z = 0;
                        set_component(x, y, z, axis, plane);
                        set_component(x, y, z, basis.u, u);
                        set_component(x, y, z, basis.v, v);
                        const std::uint8_t material = chunk.get(x, y, z);
                        if (material == 0) continue;

                        int nx = x, ny = y, nz = z;
                        set_component(nx, ny, nz, axis, plane + sign);
                        if (chunk.get(nx, ny, nz) == 0)
                            mask[static_cast<std::size_t>(v) * uSize + u] = material;
                    }
                }

                for (int v = 0; v < vSize; ++v) {
                    for (int u = 0; u < uSize;) {
                        const std::uint8_t material =
                            mask[static_cast<std::size_t>(v) * uSize + u];
                        if (material == 0) {
                            ++u;
                            continue;
                        }

                        int width = 1;
                        while (u + width < uSize &&
                               mask[static_cast<std::size_t>(v) * uSize + u + width] == material)
                            ++width;

                        int height = 1;
                        bool extend = true;
                        while (v + height < vSize && extend) {
                            for (int k = 0; k < width; ++k) {
                                if (mask[static_cast<std::size_t>(v + height) * uSize + u + k] != material) {
                                    extend = false;
                                    break;
                                }
                            }
                            if (extend) ++height;
                        }

                        MeshQuad quad{};
                        int qx = 0, qy = 0, qz = 0;
                        set_component(qx, qy, qz, axis, plane + (sign > 0 ? 1 : 0));
                        set_component(qx, qy, qz, basis.u, u);
                        set_component(qx, qy, qz, basis.v, v);
                        quad.x = static_cast<std::uint8_t>(qx);
                        quad.y = static_cast<std::uint8_t>(qy);
                        quad.z = static_cast<std::uint8_t>(qz);
                        quad.width = static_cast<std::uint8_t>(width);
                        quad.height = static_cast<std::uint8_t>(height);
                        quad.material = material;
                        quad.face = face;
                        output.push_back(quad);

                        for (int row = 0; row < height; ++row)
                            for (int column = 0; column < width; ++column)
                                mask[static_cast<std::size_t>(v + row) * uSize + u + column] = 0;
                        u += width;
                    }
                }
            }
        }
    }
}

} // namespace mineplus

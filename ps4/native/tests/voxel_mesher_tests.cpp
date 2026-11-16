#include "voxel_mesher.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace mineplus;

static int count_face(const std::vector<MeshQuad>& mesh, VoxelFace face) {
    int count = 0;
    for (const auto& quad : mesh) if (quad.face == face) ++count;
    return count;
}

static int face_index(VoxelFace face) {
    return static_cast<int>(face);
}

static bool same_quad(const MeshQuad& a, const MeshQuad& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z &&
           a.width == b.width && a.height == b.height &&
           a.material == b.material && a.face == b.face;
}

int main() {
    VoxelChunk chunk{};
    std::vector<MeshQuad> mesh;

    // Empty chunk creates no geometry.
    assert(VoxelMesher::build(chunk, mesh));
    assert(mesh.empty());

    // One isolated voxel has six exposed faces.
    assert(chunk.set(3, 4, 5, 7));
    assert(VoxelMesher::build(chunk, mesh));
    assert(mesh.size() == 6);
    assert(count_face(mesh, VoxelFace::NegativeX) == 1);
    assert(count_face(mesh, VoxelFace::PositiveX) == 1);
    assert(count_face(mesh, VoxelFace::NegativeY) == 1);
    assert(count_face(mesh, VoxelFace::PositiveY) == 1);
    assert(count_face(mesh, VoxelFace::NegativeZ) == 1);
    assert(count_face(mesh, VoxelFace::PositiveZ) == 1);

    // Two adjacent equal voxels merge into six rectangular faces, not ten.
    assert(chunk.set(4, 4, 5, 7));
    assert(VoxelMesher::build(chunk, mesh));
    assert(mesh.size() == 6);

    // Different materials remain separate on outer surfaces, but the shared
    // internal face is still culled in both directions.
    assert(chunk.set(4, 4, 5, 9));
    assert(VoxelMesher::build(chunk, mesh));
    assert(mesh.size() == 10);

    // A solid 4x3x2 cuboid becomes six quads after greedy merging.
    chunk = VoxelChunk{};
    for (int z = 8; z < 10; ++z)
        for (int y = 2; y < 5; ++y)
            for (int x = 6; x < 10; ++x)
                assert(chunk.set(x, y, z, 3));
    assert(VoxelMesher::build(chunk, mesh));
    assert(mesh.size() == 6);
    for (const auto& quad : mesh) {
        assert(quad.material == 3);
        assert(quad.width > 0 && quad.height > 0);
    }

    // Halo voxels suppress faces at all negative chunk boundaries.
    chunk = VoxelChunk{};
    assert(chunk.set(0, 0, 0, 2));
    assert(chunk.set(-1, 0, 0, 2));
    assert(chunk.set(0, -1, 0, 2));
    assert(chunk.set(0, 0, -1, 2));
    assert(VoxelMesher::build(chunk, mesh));
    assert(count_face(mesh, VoxelFace::NegativeX) == 0);
    assert(count_face(mesh, VoxelFace::NegativeY) == 0);
    assert(count_face(mesh, VoxelFace::NegativeZ) == 0);
    assert(mesh.size() == 3);

    // The positive halo is at SIZE, not at local coordinate 1.
    chunk = VoxelChunk{};
    assert(chunk.set(VoxelChunk::SIZE_X - 1, VoxelChunk::SIZE_Y - 1,
                     VoxelChunk::SIZE_Z - 1, 2));
    assert(chunk.set(VoxelChunk::SIZE_X, VoxelChunk::SIZE_Y - 1,
                     VoxelChunk::SIZE_Z - 1, 2));
    assert(chunk.set(VoxelChunk::SIZE_X - 1, VoxelChunk::SIZE_Y,
                     VoxelChunk::SIZE_Z - 1, 2));
    assert(chunk.set(VoxelChunk::SIZE_X - 1, VoxelChunk::SIZE_Y - 1,
                     VoxelChunk::SIZE_Z, 2));
    assert(VoxelMesher::build(chunk, mesh));
    assert(count_face(mesh, VoxelFace::PositiveX) == 0);
    assert(count_face(mesh, VoxelFace::PositiveY) == 0);
    assert(count_face(mesh, VoxelFace::PositiveZ) == 0);
    assert(mesh.size() == 3);

    // A mesh that exactly fills its budget is valid; only the next quad
    // beyond the budget must fail and clear the output.
    assert(VoxelMesher::build(chunk, mesh, 3));
    assert(mesh.size() == 3);

    // A caller can impose a hard geometry budget. Failure never leaves a
    // partial mesh that could be uploaded as if it were complete.
    assert(!VoxelMesher::build(chunk, mesh, 2));
    assert(mesh.empty());
    assert(!VoxelMesher::build(chunk, mesh, 0));
    assert(mesh.empty());


    // Property-style coverage: a deterministic mixed-material volume with
    // halo neighbors must preserve the exact exposed unit-face area. This
    // catches missing/duplicated greedy rectangles across all three axes.
    chunk = VoxelChunk{};
    std::uint32_t rng = 0x91e10da5u;
    auto next_random = [&rng]() {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    };
    for (int z = -1; z <= 16; ++z) {
        for (int y = -1; y <= 16; ++y) {
            for (int x = -1; x <= 16; ++x) {
                const std::uint32_t value = next_random();
                if ((value & 7u) < 3u)
                    assert(chunk.set(x, y, z,
                        static_cast<std::uint8_t>(1u + ((value >> 8) % 3u))));
            }
        }
    }

    std::array<std::array<std::uint32_t, 4>, 6> expected{};
    const int dx[6] = {-1, 1, 0, 0, 0, 0};
    const int dy[6] = {0, 0, -1, 1, 0, 0};
    const int dz[6] = {0, 0, 0, 0, -1, 1};
    for (int z = 0; z < VoxelChunk::SIZE_Z; ++z) {
        for (int y = 0; y < VoxelChunk::SIZE_Y; ++y) {
            for (int x = 0; x < VoxelChunk::SIZE_X; ++x) {
                const std::uint8_t material = chunk.get(x, y, z);
                if (material == 0) continue;
                for (int face = 0; face < 6; ++face) {
                    if (chunk.get(x + dx[face], y + dy[face],
                                  z + dz[face]) == 0)
                        ++expected[face][material];
                }
            }
        }
    }

    assert(VoxelMesher::build(chunk, mesh));
    std::array<std::array<std::uint32_t, 4>, 6> actual{};
    for (const auto& quad : mesh) {
        assert(quad.material > 0 && quad.material < 4);
        assert(quad.width > 0 && quad.height > 0);
        const int face = face_index(quad.face);
        actual[face][quad.material] +=
            static_cast<std::uint32_t>(quad.width) * quad.height;
    }
    assert(actual == expected);

    std::vector<MeshQuad> repeated;
    assert(VoxelMesher::build(chunk, repeated));
    assert(repeated.size() == mesh.size());
    for (std::size_t i = 0; i < mesh.size(); ++i)
        assert(same_quad(mesh[i], repeated[i]));

    assert(!chunk.set(-2, 0, 0, 1));
    assert(!chunk.set(0, 0, VoxelChunk::SIZE_Z + 1, 1));
    std::cout << "MINEPLUS voxel mesher tests: OK\n";
    return 0;
}

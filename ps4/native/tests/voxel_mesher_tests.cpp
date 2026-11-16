#include "voxel_mesher.hpp"

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

    // Halo voxels suppress faces at chunk boundaries, avoiding cracks and
    // duplicate internal polygons when neighboring chunks are meshed.
    chunk = VoxelChunk{};
    assert(chunk.set(0, 0, 0, 2));
    assert(chunk.set(-1, 0, 0, 2));
    assert(chunk.set(VoxelChunk::SIZE_X, 0, 0, 2));
    assert(VoxelMesher::build(chunk, mesh));
    assert(count_face(mesh, VoxelFace::NegativeX) == 0);
    assert(count_face(mesh, VoxelFace::PositiveX) == 0);
    assert(mesh.size() == 4);

    // A caller can impose a hard geometry budget. Failure never leaves a
    // partial mesh that could be uploaded as if it were complete.
    assert(!VoxelMesher::build(chunk, mesh, 3));
    assert(mesh.empty());
    assert(!VoxelMesher::build(chunk, mesh, 0));
    assert(mesh.empty());

    assert(!chunk.set(-2, 0, 0, 1));
    assert(!chunk.set(0, 0, VoxelChunk::SIZE_Z + 1, 1));
    std::cout << "MINEPLUS voxel mesher tests: OK\n";
    return 0;
}

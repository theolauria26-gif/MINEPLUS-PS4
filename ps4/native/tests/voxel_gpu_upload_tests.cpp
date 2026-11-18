#include "voxel_gpu_upload.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

using namespace mineplus;

int main() {
    std::vector<VoxelMeshVertex> source(3);
    source[0].x = 1.0f;
    source[0].y = 2.0f;
    source[0].z = 3.0f;
    source[0].u = 0.25f;
    source[0].v = 0.75f;
    source[0].normal_x = -1;
    source[0].material = 4;

    source[1].x = -8.0f;
    source[1].y = 9.0f;
    source[1].z = 10.0f;
    source[1].u = 2.0f;
    source[1].v = 5.0f;
    source[1].normal_y = 1;
    source[1].material = 22;

    source[2].normal_z = -1;
    source[2].material = 23;

    std::vector<VoxelGpuVertex> output;
    assert(VoxelGpuUploadBuilder::build(source, output));
    assert(output.size() == source.size());
    assert(output[0].x == 1.0f && output[0].y == 2.0f && output[0].z == 3.0f);
    assert(output[0].u == 0.25f && output[0].v == 0.75f);
    assert(output[0].nx == -1.0f && output[0].ny == 0.0f && output[0].nz == 0.0f);
    assert(output[0].material == 4u);
    assert(output[1].x == -8.0f && output[1].material == 22u);
    assert(output[1].ny == 1.0f && output[1].nx == 0.0f && output[1].nz == 0.0f);
    assert(output[2].nz == -1.0f && output[2].material == 23u);
    assert(sizeof(VoxelGpuVertex) == 36u);
    assert(offsetof(VoxelGpuVertex, material) == 32u);

    std::vector<VoxelGpuVertex> rejected(1);
    assert(!VoxelGpuUploadBuilder::build(source, rejected, 2));
    assert(rejected.empty());

    source[1].material = 0;
    assert(!VoxelGpuUploadBuilder::build(source, rejected));
    assert(rejected.empty());

    source[1].material = 22;
    source[1].normal_x = 1;
    source[1].normal_y = 1;
    assert(!VoxelGpuUploadBuilder::build(source, rejected));
    assert(rejected.empty());

    source[1].normal_x = 0;
    source[1].normal_y = 1;
    source[1].material = 24;
    assert(!VoxelGpuUploadBuilder::build(source, rejected));
    assert(rejected.empty());

    source[1].material = 22;
    source[1].x = std::numeric_limits<float>::infinity();
    assert(!VoxelGpuUploadBuilder::build(source, rejected));
    assert(rejected.empty());

    source[1].x = -8.0f;
    std::vector<VoxelMeshVertex> empty;
    assert(VoxelGpuUploadBuilder::build(empty, output));
    assert(output.empty());

    std::vector<std::uint32_t> indices{0u, 1u, 2u};
    std::vector<VoxelGpuVertex> mesh_vertices;
    std::vector<std::uint32_t> mesh_indices;
    assert(VoxelGpuMeshUploadBuilder::build(source, indices, mesh_vertices, mesh_indices));
    assert(mesh_vertices.size() == source.size());
    assert(mesh_indices == indices);
    assert(!VoxelGpuMeshUploadBuilder::build(source, {0u, 1u}, mesh_vertices, mesh_indices));
    assert(mesh_vertices.empty() && mesh_indices.empty());
    assert(!VoxelGpuMeshUploadBuilder::build(source, {0u, 1u, 3u}, mesh_vertices, mesh_indices));
    assert(mesh_vertices.empty() && mesh_indices.empty());
    assert(!VoxelGpuMeshUploadBuilder::build(source, indices, mesh_vertices, mesh_indices, 2));
    assert(mesh_vertices.empty() && mesh_indices.empty());
    assert(!VoxelGpuMeshUploadBuilder::build(source, indices, mesh_vertices, mesh_indices, 3, 2));
    assert(mesh_vertices.empty() && mesh_indices.empty());

    std::cout << "MINEPLUS voxel GPU upload staging tests: OK\n";
}

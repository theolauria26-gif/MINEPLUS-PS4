#include "voxel_gpu_upload.hpp"
#include "material_atlas.hpp"
#include <cmath>
#include <limits>

namespace mineplus {
namespace {
bool valid_vertex(const VoxelMeshVertex& vertex) {
    MaterialAtlasRegion region{};
    if (!material_atlas_region(vertex.material, region)) return false;
    if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
        !std::isfinite(vertex.z) || !std::isfinite(vertex.u) ||
        !std::isfinite(vertex.v)) return false;

    const int nx = vertex.normal_x;
    const int ny = vertex.normal_y;
    const int nz = vertex.normal_z;
    const int nonzero_axes = (nx != 0) + (ny != 0) + (nz != 0);
    return nonzero_axes == 1 &&
           (nx == -1 || nx == 0 || nx == 1) &&
           (ny == -1 || ny == 0 || ny == 1) &&
           (nz == -1 || nz == 0 || nz == 1);
}
} // namespace

bool VoxelGpuUploadBuilder::build(
    const std::vector<VoxelMeshVertex>& source,
    std::vector<VoxelGpuVertex>& output,
    std::size_t max_vertices) {
    output.clear();
    if (source.size() > max_vertices ||
        source.size() > std::numeric_limits<std::size_t>::max() /
                            sizeof(VoxelGpuVertex)) {
        return false;
    }

    output.reserve(source.size());
    for (const VoxelMeshVertex& vertex : source) {
        if (!valid_vertex(vertex)) {
            output.clear();
            return false;
        }

        VoxelGpuVertex packed{};
        packed.x = vertex.x;
        packed.y = vertex.y;
        packed.z = vertex.z;
        packed.u = vertex.u;
        packed.v = vertex.v;
        packed.nx = static_cast<float>(vertex.normal_x);
        packed.ny = static_cast<float>(vertex.normal_y);
        packed.nz = static_cast<float>(vertex.normal_z);
        packed.material = vertex.material;
        output.push_back(packed);
    }
    return true;
}

} // namespace mineplus

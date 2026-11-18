#include "voxel_gpu_upload.hpp"
#include <limits>

namespace mineplus {

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
        if (vertex.material == 0) {
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

#include "voxel_mesh_indices.hpp"
#include <limits>
namespace mineplus {
bool VoxelMeshIndexBuilder::build(const std::vector<MeshQuad>& quads,
    std::size_t vertex_count, std::vector<std::uint32_t>& output,
    std::size_t max_indices) {
    output.clear();
    if (quads.size() > std::numeric_limits<std::uint32_t>::max() / 4u ||
        quads.size() > std::numeric_limits<std::size_t>::max() / 6u ||
        vertex_count != quads.size() * 4u ||
        quads.size() * 6u > max_indices) return false;
    output.reserve(quads.size() * 6u);
    for (std::size_t q = 0; q < quads.size(); ++q) {
        const std::uint32_t base = static_cast<std::uint32_t>(q * 4u);
        output.push_back(base);
        output.push_back(base + 1u);
        output.push_back(base + 2u);
        output.push_back(base);
        output.push_back(base + 2u);
        output.push_back(base + 3u);
    }
    return true;
}
} // namespace mineplus

#include "world_runtime.hpp"
#include "save_codec.hpp"
#include <array>

namespace mineplus {

WorldRuntime::WorldRuntime(std::uint64_t seed) : chunks_(seed) {}

void WorldRuntime::update_center(std::int32_t world_x, std::int32_t world_y,
                                 std::int32_t radius_chunks) {
    chunks_.update_center(world_x, world_y, radius_chunks);
}

bool WorldRuntime::get_tile(std::int32_t world_x, std::int32_t world_y,
                            std::uint8_t& tile) {
    SurfaceSample sample{};
    if (!chunks_.get(world_x, world_y, sample)) return false;
    tile = sample.surface;
    return true;
}

bool WorldRuntime::get_sample(std::int32_t world_x, std::int32_t world_y,
                               SurfaceSample& sample) {
    return chunks_.get(world_x, world_y, sample);
}

bool WorldRuntime::get_voxel(std::int32_t world_x, std::int32_t world_y,
                              std::int32_t z, VoxelSample& voxel) {
    return chunks_.get_voxel(world_x, world_y, z, voxel);
}

bool WorldRuntime::get_structure(std::int32_t world_x, std::int32_t world_y,
                                 StructureAnchor& anchor) {
    return chunks_.get_structure(world_x, world_y, anchor);
}

bool WorldRuntime::set_tile(std::int32_t world_x, std::int32_t world_y,
                            std::uint8_t tile) {
    return chunks_.set_surface(world_x, world_y, tile);
}

std::size_t WorldRuntime::export_edits(ChunkCache::Edit* output,
                                       std::size_t capacity) const {
    return chunks_.export_edits(output, capacity);
}

bool WorldRuntime::import_edits(const ChunkCache::Edit* edits,
                                std::size_t count) {
    return chunks_.import_edits(edits, count);
}

std::size_t WorldRuntime::loaded_chunks() const { return chunks_.loaded_count(); }
std::size_t WorldRuntime::saved_edits() const { return chunks_.delta_count(); }
std::uint64_t WorldRuntime::seed() const { return chunks_.seed(); }

bool WorldRuntime::encode_save(std::vector<std::uint8_t>& bytes) const {
    std::array<ChunkCache::Edit, ChunkCache::DELTA_CAPACITY> edits{};
    const std::size_t count = chunks_.export_edits(edits.data(), edits.size());
    if (count != chunks_.delta_count()) return false;
    WorldArchive archive;
    archive.seed = chunks_.seed();
    archive.edits.assign(edits.begin(), edits.begin() + count);
    return SaveCodec::encode(archive, bytes);
}

bool WorldRuntime::decode_save(const std::uint8_t* bytes, std::size_t size) {
    WorldArchive archive;
    if (!SaveCodec::decode(bytes, size, archive)) return false;
    return chunks_.restore(archive.seed, archive.edits.data(),
                           archive.edits.size());
}

} // namespace mineplus

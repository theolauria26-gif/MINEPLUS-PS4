#pragma once
#include "chunk_cache.hpp"
#include "dungeon_loot.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mineplus {

// Gameplay-facing interface for the streamed world. Coordinates are signed
// world-tile coordinates; callers never calculate chunk or local coordinates.
class WorldRuntime {
public:
    explicit WorldRuntime(std::uint64_t seed = 4839201ULL);

    void update_center(std::int32_t world_x, std::int32_t world_y,
                       std::int32_t radius_chunks = 4);
    bool get_tile(std::int32_t world_x, std::int32_t world_y,
                  std::uint8_t& tile);
    bool get_sample(std::int32_t world_x, std::int32_t world_y,
                    SurfaceSample& sample);
    bool get_voxel(std::int32_t world_x, std::int32_t world_y,
                   std::int32_t z, VoxelSample& voxel);
    bool get_structure(std::int32_t world_x, std::int32_t world_y,
                       StructureAnchor& anchor);
    bool set_tile(std::int32_t world_x, std::int32_t world_y,
                  std::uint8_t tile);
    bool set_voxel(std::int32_t world_x, std::int32_t world_y, std::int32_t z,
                   VoxelMaterial material);
    std::size_t export_voxel_edits(ChunkCache::VoxelEdit* output,
                                   std::size_t capacity) const;
    bool import_voxel_edits(const ChunkCache::VoxelEdit* edits, std::size_t count);

    std::size_t export_edits(ChunkCache::Edit* output,
                             std::size_t capacity) const;
    bool import_edits(const ChunkCache::Edit* edits, std::size_t count);

    std::size_t loaded_chunks() const;
    std::size_t saved_edits() const;
    std::uint64_t seed() const;

    // Persistent dungeon chest state keyed by stable dungeon room ID.
    bool set_dungeon_chest(const DungeonChestState& chest);
    bool get_dungeon_chest(std::uint32_t room_id, DungeonChestState& chest) const;
    const std::vector<DungeonChestState>& dungeon_chests() const;

    // Portable, checksummed snapshot of the seed and player-made terrain edits.
    bool encode_save(std::vector<std::uint8_t>& bytes) const;
    bool decode_save(const std::uint8_t* bytes, std::size_t size);

private:
    ChunkCache chunks_;
    std::vector<DungeonChestState> dungeon_chests_;
};

} // namespace mineplus

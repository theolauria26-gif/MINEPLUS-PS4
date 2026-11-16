#pragma once
#include "chunk_generator.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace mineplus {

// Fixed-memory LRU cache: predictable allocation and no heap churn on PS4.
// 9x9 chunks cover a 144x144 tile area around the player.
class ChunkCache {
public:
    static constexpr std::size_t CAPACITY = 81;
    static constexpr std::size_t DELTA_CAPACITY = 2048;
    static constexpr std::size_t VOXEL_DELTA_CAPACITY = 4096;

    explicit ChunkCache(std::uint64_t seed = 4839201ULL);

    // Loads a 3x3/5x5/9x9 neighborhood, keeping the nearest chunks resident.
    void update_center(std::int32_t world_x, std::int32_t world_y,
                       std::int32_t radius_chunks = 4);
    bool get(std::int32_t world_x, std::int32_t world_y,
             SurfaceSample& out);
    bool get_voxel(std::int32_t world_x, std::int32_t world_y,
                   std::int32_t z, VoxelSample& out);
    bool get_structure(std::int32_t world_x, std::int32_t world_y,
                       StructureAnchor& out);
    bool set_surface(std::int32_t world_x, std::int32_t world_y,
                     std::uint8_t tile);
    // Compact edit journal transfer for durable save files.
    struct Edit { std::int32_t x, y; std::uint8_t tile; };
    struct VoxelEdit { std::int32_t x, y, z; VoxelMaterial material; };
    bool set_voxel(std::int32_t world_x, std::int32_t world_y, std::int32_t z,
                   VoxelMaterial material);
    std::size_t export_edits(Edit* output, std::size_t capacity) const;
    bool import_edits(const Edit* edits, std::size_t count);
    std::size_t export_voxel_edits(VoxelEdit* output, std::size_t capacity) const;
    bool import_voxel_edits(const VoxelEdit* edits, std::size_t count);
    bool restore(std::uint64_t seed, const Edit* edits, std::size_t count,
                 const VoxelEdit* voxel_edits = nullptr, std::size_t voxel_count = 0);

    std::size_t loaded_count() const;
    std::size_t delta_count() const;
    std::uint64_t seed() const { return generator_seed_; }

private:
    struct Entry {
        ChunkData data{};
        std::uint64_t stamp = 0;
        bool valid = false;
    };
    struct Delta {
        std::int32_t x = 0, y = 0;
        std::uint8_t tile = 0;
        bool valid = false;
    };
    struct VoxelDelta {
        std::int32_t x = 0, y = 0, z = 0;
        VoxelMaterial material = VoxelMaterial::Air;
        bool valid = false;
    };

    ChunkGenerator generator_;
    std::uint64_t generator_seed_;
    std::array<Entry, CAPACITY> entries_{};
    std::array<Delta, DELTA_CAPACITY> deltas_{};
    std::array<VoxelDelta, VOXEL_DELTA_CAPACITY> voxel_deltas_{};
    std::uint64_t clock_ = 1;

    Entry* find_chunk(std::int32_t cx, std::int32_t cy);
    Entry* acquire_chunk(std::int32_t cx, std::int32_t cy);
    void apply_deltas(ChunkData& chunk);
    static std::size_t local_index(std::int32_t x, std::int32_t y);
};

} // namespace mineplus

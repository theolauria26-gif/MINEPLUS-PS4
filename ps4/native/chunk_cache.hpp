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

    explicit ChunkCache(std::uint64_t seed = 4839201ULL);

    // Loads a 3x3/5x5/9x9 neighborhood, keeping the nearest chunks resident.
    void update_center(std::int32_t world_x, std::int32_t world_y,
                       std::int32_t radius_chunks = 4);
    bool get(std::int32_t world_x, std::int32_t world_y,
             SurfaceSample& out);
    bool set_surface(std::int32_t world_x, std::int32_t world_y,
                     std::uint8_t tile);
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

    ChunkGenerator generator_;
    std::uint64_t generator_seed_;
    std::array<Entry, CAPACITY> entries_{};
    std::array<Delta, DELTA_CAPACITY> deltas_{};
    std::uint64_t clock_ = 1;

    Entry* find_chunk(std::int32_t cx, std::int32_t cy);
    Entry* acquire_chunk(std::int32_t cx, std::int32_t cy);
    void apply_deltas(ChunkData& chunk);
    static std::size_t local_index(std::int32_t x, std::int32_t y);
};

} // namespace mineplus

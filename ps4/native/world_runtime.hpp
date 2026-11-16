#pragma once
#include "chunk_cache.hpp"
#include <cstddef>
#include <cstdint>

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
    bool set_tile(std::int32_t world_x, std::int32_t world_y,
                  std::uint8_t tile);

    std::size_t export_edits(ChunkCache::Edit* output,
                             std::size_t capacity) const;
    bool import_edits(const ChunkCache::Edit* edits, std::size_t count);

    std::size_t loaded_chunks() const;
    std::size_t saved_edits() const;
    std::uint64_t seed() const;

private:
    ChunkCache chunks_;
};

} // namespace mineplus

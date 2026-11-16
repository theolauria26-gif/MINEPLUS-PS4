#pragma once
#include "chunk_cache.hpp"
#include <cstdint>
#include <vector>

namespace mineplus {

struct WorldArchive {
    std::uint64_t seed = 4839201ULL;
    std::vector<ChunkCache::Edit> edits;
};

class SaveCodec {
public:
    static constexpr std::uint32_t VERSION = 1;
    static constexpr std::size_t MAX_EDITS = ChunkCache::DELTA_CAPACITY;

    // Stable little-endian format; never writes compiler struct padding.
    static bool encode(const WorldArchive& archive, std::vector<std::uint8_t>& bytes);
    static bool decode(const std::uint8_t* bytes, std::size_t size, WorldArchive& archive);
};

} // namespace mineplus

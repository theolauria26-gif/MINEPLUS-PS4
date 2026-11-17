#pragma once
#include "dungeon_graph.hpp"
#include "voxel_mesher.hpp"
#include <cstdint>

namespace mineplus {

struct DungeonVoxelSettings {
    std::int32_t chunk_origin_x = 0;
    std::int32_t chunk_origin_y = 0;
    std::int32_t floor_z = 32;
    std::uint8_t wall_height = 5;
    std::uint8_t wall_material = 8;
    std::uint8_t floor_material = 3;
    bool build_ceiling = false;
};

// Stamps rooms and their connected corridors into one clipped 16x16x128 chunk.
// The output chunk is cleared before a valid layout is assembled.
class DungeonVoxelBuilder {
public:
    static bool build_room(const DungeonRoom& room,
                           const DungeonVoxelSettings& settings,
                           VoxelChunk& output);
    static bool build_layout_chunk(const DungeonLayout& layout,
                                   const DungeonVoxelSettings& settings,
                                   VoxelChunk& output);
};

} // namespace mineplus

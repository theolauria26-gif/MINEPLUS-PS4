#pragma once
#include <cstdint>
#include <vector>

namespace mineplus {

enum class DungeonRoomType : std::uint8_t {
    Start = 0,
    Combat = 1,
    Treasure = 2,
    Puzzle = 3,
    Boss = 4,
    Exit = 5
};

struct DungeonRoom {
    std::uint32_t id = 0;
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint8_t width = 0;
    std::uint8_t height = 0;
    DungeonRoomType type = DungeonRoomType::Combat;
};

struct DungeonConnection {
    std::uint32_t from = 0;
    std::uint32_t to = 0;
    std::uint8_t corridor_width = 2;
};

struct DungeonLayout {
    std::uint64_t seed = 0;
    std::vector<DungeonRoom> rooms;
    std::vector<DungeonConnection> connections;
};

// Builds a deterministic, connected room graph. Coordinates are in dungeon
// tile units; a later voxel builder can extrude rooms and corridors from it.
// Requests outside [4, max_rooms] fail and leave output empty.
class DungeonGraphGenerator {
public:
    static bool generate(std::uint64_t seed, std::uint32_t requested_rooms,
                         DungeonLayout& output,
                         std::uint32_t max_rooms = 32);
};

} // namespace mineplus

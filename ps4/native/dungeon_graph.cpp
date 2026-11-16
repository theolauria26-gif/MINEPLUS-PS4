#include "dungeon_graph.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace mineplus {
namespace {

class Rng {
public:
    explicit Rng(std::uint64_t seed) : state_(seed) {}
    std::uint32_t next() {
        state_ += 0x9e3779b97f4a7c15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return static_cast<std::uint32_t>((z ^ (z >> 31)) >> 32);
    }
    std::uint32_t range(std::uint32_t bound) {
        return bound ? next() % bound : 0;
    }
private:
    std::uint64_t state_;
};

struct Cell { std::int32_t x, y; };
constexpr std::int32_t kGridStep = 18;
constexpr std::array<Cell, 4> kDirections = {{{1,0},{0,1},{-1,0},{0,-1}}};

bool occupied(const std::vector<DungeonRoom>& rooms,
              std::int32_t gx, std::int32_t gy) {
    for (const DungeonRoom& room : rooms)
        if (room.grid_x == gx && room.grid_y == gy) return true;
    return false;
}

bool connected_pair(const DungeonConnection& edges,
                    std::uint32_t a, std::uint32_t b) {
    return (edges.from == a && edges.to == b) ||
           (edges.from == b && edges.to == a);
}

} // namespace

bool DungeonGraphGenerator::generate(std::uint64_t seed,
                                     std::uint32_t requested_rooms,
                                     DungeonLayout& output,
                                     std::uint32_t max_rooms) {
    output = DungeonLayout{};
    if (max_rooms < 4 || max_rooms > 256 ||
        requested_rooms < 4 || requested_rooms > max_rooms) return false;

    Rng rng(seed);
    output.seed = seed;
    output.rooms.reserve(requested_rooms);
    output.connections.reserve(requested_rooms * 2);

    DungeonRoom first;
    first.id = 0;
    first.width = static_cast<std::uint8_t>(7 + rng.range(5));
    first.height = static_cast<std::uint8_t>(7 + rng.range(5));
    first.type = DungeonRoomType::Start;
    output.rooms.push_back(first);

    for (std::uint32_t id = 1; id < requested_rooms; ++id) {
        bool placed = false;
        for (std::uint32_t attempt = 0; attempt < 256 && !placed; ++attempt) {
            const std::uint32_t parent = rng.range(
                static_cast<std::uint32_t>(output.rooms.size()));
            const Cell direction = kDirections[rng.range(4)];
            const DungeonRoom& base = output.rooms[parent];
            const std::int32_t gx = base.grid_x + direction.x;
            const std::int32_t gy = base.grid_y + direction.y;
            if (occupied(output.rooms, gx, gy)) continue;

            DungeonRoom room;
            room.id = id;
            room.grid_x = gx;
            room.grid_y = gy;
            room.x = gx * kGridStep;
            room.y = gy * kGridStep;
            room.width = static_cast<std::uint8_t>(7 + rng.range(5));
            room.height = static_cast<std::uint8_t>(7 + rng.range(5));
            room.type = DungeonRoomType::Combat;
            output.rooms.push_back(room);
            output.connections.push_back({parent, id, 2});
            placed = true;
        }
        if (!placed) {
            output = DungeonLayout{};
            return false;
        }
    }

    const std::uint32_t boss_id = requested_rooms - 2;
    const std::uint32_t exit_id = requested_rooms - 1;
    output.rooms[boss_id].type = DungeonRoomType::Boss;
    output.rooms[exit_id].type = DungeonRoomType::Exit;
    if (requested_rooms >= 5)
        output.rooms[1 + rng.range(requested_rooms - 3)].type =
            DungeonRoomType::Treasure;
    if (requested_rooms >= 6) {
        std::uint32_t puzzle = 1 + rng.range(requested_rooms - 3);
        if (puzzle == 1) puzzle = 2;
        if (puzzle != boss_id) output.rooms[puzzle].type = DungeonRoomType::Puzzle;
    }

    // Add optional loops between adjacent rooms without duplicate edges.
    for (std::size_t i = 0; i < output.rooms.size(); ++i) {
        for (std::size_t j = i + 1; j < output.rooms.size(); ++j) {
            const DungeonRoom& a = output.rooms[i];
            const DungeonRoom& b = output.rooms[j];
            const int distance = (a.grid_x > b.grid_x ? a.grid_x - b.grid_x : b.grid_x - a.grid_x) +
                                 (a.grid_y > b.grid_y ? a.grid_y - b.grid_y : b.grid_y - a.grid_y);
            if (distance == 1 && rng.range(100) < 22 &&
                !connected_pair(output.connections[0], 0, 0)) {
                bool exists = false;
                for (const DungeonConnection& edge : output.connections)
                    if (connected_pair(edge, a.id, b.id)) { exists = true; break; }
                if (!exists) output.connections.push_back({a.id, b.id, 2});
            }
        }
    }
    return true;
}

} // namespace mineplus

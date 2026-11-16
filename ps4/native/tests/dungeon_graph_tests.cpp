#include "dungeon_graph.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace mineplus;

static bool reachable(const DungeonLayout& layout) {
    if (layout.rooms.empty()) return false;
    std::vector<bool> seen(layout.rooms.size(), false);
    std::vector<std::uint32_t> queue(1, 0);
    seen[0] = true;
    for (std::size_t head = 0; head < queue.size(); ++head) {
        for (const DungeonConnection& edge : layout.connections) {
            std::uint32_t next = UINT32_MAX;
            if (edge.from == queue[head]) next = edge.to;
            else if (edge.to == queue[head]) next = edge.from;
            if (next < seen.size() && !seen[next]) {
                seen[next] = true;
                queue.push_back(next);
            }
        }
    }
    for (bool value : seen) if (!value) return false;
    return true;
}

int main() {
    DungeonLayout a, b;
    assert(DungeonGraphGenerator::generate(0xC0FFEEULL, 18, a));
    assert(DungeonGraphGenerator::generate(0xC0FFEEULL, 18, b));
    assert(a.rooms.size() == 18 && a.connections.size() >= 17);
    assert(a.rooms[0].type == DungeonRoomType::Start);
    assert(a.rooms[16].type == DungeonRoomType::Boss);
    assert(a.rooms[17].type == DungeonRoomType::Exit);
    assert(reachable(a));
    assert(a.rooms.size() == b.rooms.size());
    assert(a.connections.size() == b.connections.size());
    for (std::size_t i = 0; i < a.rooms.size(); ++i) {
        assert(a.rooms[i].grid_x == b.rooms[i].grid_x);
        assert(a.rooms[i].grid_y == b.rooms[i].grid_y);
        assert(a.rooms[i].width == b.rooms[i].width);
        assert(a.rooms[i].height == b.rooms[i].height);
        assert(a.rooms[i].type == b.rooms[i].type);
    }
    for (std::size_t i = 0; i < a.connections.size(); ++i) {
        assert(a.connections[i].from == b.connections[i].from);
        assert(a.connections[i].to == b.connections[i].to);
        assert(a.connections[i].from < a.rooms.size());
        assert(a.connections[i].to < a.rooms.size());
        assert(a.connections[i].from != a.connections[i].to);
        assert(a.connections[i].corridor_width >= 1);
    }
    for (std::size_t i = 0; i < a.rooms.size(); ++i)
        for (std::size_t j = i + 1; j < a.rooms.size(); ++j)
            assert(a.rooms[i].grid_x != a.rooms[j].grid_x ||
                   a.rooms[i].grid_y != a.rooms[j].grid_y);

    DungeonLayout invalid;
    assert(!DungeonGraphGenerator::generate(1, 3, invalid));
    assert(invalid.rooms.empty() && invalid.connections.empty());
    assert(!DungeonGraphGenerator::generate(1, 10, invalid, 8));
    assert(invalid.rooms.empty());
    std::cout << "MINEPLUS dungeon graph tests: OK\n";
}

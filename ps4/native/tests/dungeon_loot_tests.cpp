#include "dungeon_loot.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace mineplus;

static bool same(const std::vector<DungeonLootDrop>& a, const std::vector<DungeonLootDrop>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].item != b[i].item || a[i].count != b[i].count) return false;
    return true;
}

int main() {
    DungeonRoom room{};
    std::vector<DungeonLootDrop> first, second;
    room.id = 42;
    room.type = DungeonRoomType::Treasure;
    assert(DungeonLootGenerator::generate(0x12345678ULL, room, first));
    assert(DungeonLootGenerator::generate(0x12345678ULL, room, second));
    assert(!first.empty() && first.size() <= 6 && same(first, second));
    for (const auto& drop : first) assert(drop.count > 0 && drop.count <= 999);

    room.type = DungeonRoomType::Boss;
    assert(DungeonLootGenerator::generate(7, room, first));
    bool has_sigil = false;
    for (const auto& drop : first) if (drop.item == DungeonLootItem::BossSigil && drop.count == 1) has_sigil = true;
    assert(has_sigil && first.size() <= 6);

    room.type = DungeonRoomType::Start;
    assert(DungeonLootGenerator::generate(7, room, first) && first.empty());
    room.type = DungeonRoomType::Exit;
    assert(DungeonLootGenerator::generate(7, room, first) && first.empty());
    room.type = static_cast<DungeonRoomType>(255);
    assert(!DungeonLootGenerator::generate(7, room, first) && first.empty());
    room.type = DungeonRoomType::Combat;
    assert(!DungeonLootGenerator::generate(7, room, first, 0) && first.empty());
    assert(!DungeonLootGenerator::generate(7, room, first, 33) && first.empty());

    bool observed_variation = false;
    room.id = 43;
    assert(DungeonLootGenerator::generate(0x12345678ULL, room, first));
    room.id = 44;
    assert(DungeonLootGenerator::generate(0x12345678ULL, room, second));
    observed_variation = !same(first, second);
    assert(observed_variation);
    std::cout << "MINEPLUS dungeon loot tests: OK\n";
}

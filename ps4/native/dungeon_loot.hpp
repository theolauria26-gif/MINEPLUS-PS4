#pragma once
#include "dungeon_graph.hpp"
#include <cstdint>
#include <vector>

namespace mineplus {

enum class DungeonLootItem : std::uint16_t {
    CopperCoin = 1,
    HealingHerb = 2,
    BoneShard = 3,
    IronOre = 4,
    CrystalShard = 5,
    RuneFragment = 6,
    EmberCore = 7,
    AncientRelic = 8,
    BossSigil = 9
};

struct DungeonLootDrop {
    DungeonLootItem item = DungeonLootItem::CopperCoin;
    std::uint16_t count = 0;
};

// Stable per world/room loot. Repeated generation for the same room never rerolls.
class DungeonLootGenerator {
public:
    static bool generate(std::uint64_t world_seed, const DungeonRoom& room,
                         std::vector<DungeonLootDrop>& output,
                         std::uint8_t max_drops = 6);
};

} // namespace mineplus

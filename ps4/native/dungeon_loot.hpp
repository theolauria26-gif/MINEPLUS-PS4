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

struct DungeonChestState {
    std::uint32_t room_id = 0;
    bool opened = false;
    std::vector<DungeonLootDrop> contents;
};

// Stable per world/room loot. Repeated generation for the same room never rerolls.
class DungeonLootGenerator {
public:
    static bool generate(std::uint64_t world_seed, const DungeonRoom& room,
                         std::vector<DungeonLootDrop>& output,
                         std::uint8_t max_drops = 6);
    static bool generate_chest(std::uint64_t world_seed, const DungeonRoom& room,
                               DungeonChestState& output,
                               std::uint8_t max_drops = 6);
};

} // namespace mineplus

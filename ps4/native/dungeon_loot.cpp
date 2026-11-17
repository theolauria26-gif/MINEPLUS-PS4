#include "dungeon_loot.hpp"
#include <algorithm>
#include <array>
#include <limits>

namespace mineplus {
namespace {

struct LootRule {
    DungeonLootItem item;
    std::uint16_t weight;
    std::uint8_t minimum;
    std::uint8_t maximum;
};

std::uint64_t mix64(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

std::uint32_t bounded(std::uint64_t& state, std::uint32_t bound) {
    state = mix64(state);
    return bound ? static_cast<std::uint32_t>(state % bound) : 0U;
}

template <std::size_t N>
void roll_table(std::uint64_t& state, const std::array<LootRule, N>& rules,
                std::uint8_t rolls, std::uint8_t max_drops,
                std::vector<DungeonLootDrop>& output) {
    for (std::uint8_t roll = 0; roll < rolls && output.size() < max_drops; ++roll) {
        std::uint32_t total = 0;
        for (const LootRule& rule : rules) total += rule.weight;
        if (!total) return;
        std::uint32_t pick = bounded(state, total);
        const LootRule* chosen = &rules.back();
        for (const LootRule& rule : rules) {
            if (pick < rule.weight) { chosen = &rule; break; }
            pick -= rule.weight;
        }
        const std::uint16_t count = static_cast<std::uint16_t>(
            chosen->minimum + bounded(state, chosen->maximum - chosen->minimum + 1U));
        auto found = std::find_if(output.begin(), output.end(), [&](const DungeonLootDrop& drop) {
            return drop.item == chosen->item;
        });
        if (found != output.end()) {
            const std::uint32_t sum = static_cast<std::uint32_t>(found->count) + count;
            found->count = static_cast<std::uint16_t>(std::min<std::uint32_t>(sum, 999U));
        } else {
            output.push_back({chosen->item, count});
        }
    }
}

} // namespace

bool DungeonLootGenerator::generate(std::uint64_t world_seed, const DungeonRoom& room,
                                    std::vector<DungeonLootDrop>& output,
                                    std::uint8_t max_drops) {
    output.clear();
    if (room.type > DungeonRoomType::Exit || max_drops == 0 || max_drops > 32) return false;
    if (room.type == DungeonRoomType::Start || room.type == DungeonRoomType::Exit) return true;

    std::uint64_t state = mix64(world_seed ^ (static_cast<std::uint64_t>(room.id) << 32U) ^
                                (static_cast<std::uint64_t>(room.type) * 0xD6E8FEB86659FD93ULL));
    switch (room.type) {
    case DungeonRoomType::Combat: {
        static constexpr std::array<LootRule, 4> rules{{
            {DungeonLootItem::CopperCoin, 45, 2, 8},
            {DungeonLootItem::BoneShard, 30, 1, 4},
            {DungeonLootItem::HealingHerb, 18, 1, 2},
            {DungeonLootItem::IronOre, 7, 1, 2}
        }};
        roll_table(state, rules, static_cast<std::uint8_t>(1 + bounded(state, 3)), max_drops, output);
        break;
    }
    case DungeonRoomType::Treasure: {
        static constexpr std::array<LootRule, 5> rules{{
            {DungeonLootItem::CopperCoin, 25, 12, 30},
            {DungeonLootItem::IronOre, 25, 3, 8},
            {DungeonLootItem::CrystalShard, 22, 2, 6},
            {DungeonLootItem::RuneFragment, 18, 1, 3},
            {DungeonLootItem::EmberCore, 10, 1, 2}
        }};
        roll_table(state, rules, static_cast<std::uint8_t>(3 + bounded(state, 3)), max_drops, output);
        break;
    }
    case DungeonRoomType::Puzzle: {
        static constexpr std::array<LootRule, 3> rules{{
            {DungeonLootItem::RuneFragment, 55, 1, 3},
            {DungeonLootItem::CrystalShard, 35, 1, 3},
            {DungeonLootItem::HealingHerb, 10, 1, 2}
        }};
        roll_table(state, rules, static_cast<std::uint8_t>(1 + bounded(state, 2)), max_drops, output);
        break;
    }
    case DungeonRoomType::Boss: {
        static constexpr std::array<LootRule, 4> rules{{
            {DungeonLootItem::BossSigil, 100, 1, 1},
            {DungeonLootItem::AncientRelic, 70, 1, 2},
            {DungeonLootItem::EmberCore, 55, 2, 5},
            {DungeonLootItem::CrystalShard, 35, 4, 10}
        }};
        roll_table(state, rules, static_cast<std::uint8_t>(4 + bounded(state, 3)), max_drops, output);
        auto sigil = std::find_if(output.begin(), output.end(), [](const DungeonLootDrop& drop) {
            return drop.item == DungeonLootItem::BossSigil;
        });
        if (sigil == output.end()) {
            if (output.size() < max_drops) output.push_back({DungeonLootItem::BossSigil, 1});
            else output.back() = {DungeonLootItem::BossSigil, 1};
        }
        break;
    }
    case DungeonRoomType::Start:
    case DungeonRoomType::Exit:
        break;
    }
    return true;
}

bool DungeonLootGenerator::generate_chest(std::uint64_t world_seed, const DungeonRoom& room,
                                          DungeonChestState& output, std::uint8_t max_drops) {
    DungeonChestState candidate;
    candidate.room_id=room.id;
    candidate.opened=false;
    if(!generate(world_seed,room,candidate.contents,max_drops)) return false;
    output=std::move(candidate);
    return true;
}

} // namespace mineplus

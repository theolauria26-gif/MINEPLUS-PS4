#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace mineplus {

enum class EquipmentTier : std::uint8_t { Primitive=0, Iron=1, Obsidian=2, Runic=3, Primordial=4 };
enum class EquipmentSlot : std::uint8_t { Weapon=0, Head=1, Chest=2, Legs=3, Boots=4, Count=5 };
enum class EquipmentId : std::uint8_t {
    None=0,
    StoneHatchet, BoneCap, HideVest, TrailLeggings, MossBoots,
    IronSword, IronHelm, IronChest, IronLegs, IronBoots,
    ObsidianBlade, ObsidianHelm, ObsidianChest, ObsidianLegs, ObsidianBoots,
    RuneSword, RuneHelm, RuneChest, RuneLegs, RuneBoots,
    PrimordialBlade, PrimordialHelm, PrimordialChest, PrimordialLegs, PrimordialBoots,
    Count
};

struct EquipmentStats {
    std::uint16_t damage=0;
    std::uint16_t defense=0;
    std::uint16_t vitality=0;
    std::uint16_t arcane=0;
    std::uint16_t resistance=0;
};

struct EquipmentDefinition {
    EquipmentId id=EquipmentId::None;
    EquipmentTier tier=EquipmentTier::Primitive;
    EquipmentSlot slot=EquipmentSlot::Weapon;
    EquipmentStats stats{};
    std::uint8_t set_id=0;
    std::uint16_t durability=0;
};

struct EquipmentIngredient { std::uint8_t inventory_index=0; std::uint16_t count=0; };
struct EquipmentRecipe {
    EquipmentId output=EquipmentId::None;
    std::array<EquipmentIngredient,4> ingredients{};
    std::uint8_t ingredient_count=0;
};

struct EquipmentLoadout {
    std::array<EquipmentId,static_cast<std::size_t>(EquipmentSlot::Count)> slots{};
};

struct EquipmentState {
    std::array<std::uint8_t,static_cast<std::size_t>(EquipmentId::Count)> owned{};
    std::array<EquipmentId,static_cast<std::size_t>(EquipmentSlot::Count)> equipped{};
    std::array<std::uint16_t,static_cast<std::size_t>(EquipmentId::Count)> durability{};
};

struct EquipmentTotals {
    std::uint16_t damage=0;
    std::uint16_t defense=0;
    std::uint16_t max_health=10;
    std::uint16_t arcane=0;
    std::uint16_t resistance=0;
    std::uint8_t active_set=0;
    std::uint8_t set_piece_count=0;
};

class EquipmentCatalog {
public:
    static constexpr std::size_t count=static_cast<std::size_t>(EquipmentId::Count);
    static const EquipmentDefinition* find(EquipmentId id);
    static const EquipmentRecipe* recipe(EquipmentId id);
    static bool can_craft(EquipmentId id,const int* inventory,std::size_t inventory_count);
    static bool consume_recipe(EquipmentId id,int* inventory,std::size_t inventory_count);
    static EquipmentTotals calculate(const EquipmentLoadout& loadout);
    static bool craft(EquipmentState& state,EquipmentId id,int* inventory,std::size_t inventory_count);
    static bool equip(EquipmentState& state,EquipmentId id);
    static EquipmentTotals calculate(const EquipmentState& state);
    static void wear(EquipmentState& state,EquipmentSlot slot,std::uint16_t amount=1);
    static std::uint16_t mitigate_damage(std::uint16_t incoming,const EquipmentTotals& totals,bool elemental=false);
};

} // namespace mineplus

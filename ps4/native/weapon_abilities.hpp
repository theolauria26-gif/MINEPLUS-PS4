#pragma once
#include "equipment.hpp"
#include <cstdint>

namespace mineplus {

enum class WeaponArt : std::uint8_t { None=0, HeavyStrike, IronCleave, ObsidianRift, RunicArc, PrimordialNova };

struct WeaponArtProfile {
    WeaponArt art=WeaponArt::None;
    std::uint16_t cooldown_ms=0;
    std::uint16_t bonus_damage=0;
    std::uint8_t range=0;
    std::uint8_t durability_cost=0;
    const char* name="NONE";
};

inline WeaponArtProfile weapon_art(EquipmentId id) {
    switch(id) {
        case EquipmentId::StoneHatchet: return {WeaponArt::HeavyStrike,1300,2,1,2,"HEAVY STRIKE"};
        case EquipmentId::IronSword: return {WeaponArt::IronCleave,1600,4,2,2,"IRON CLEAVE"};
        case EquipmentId::ObsidianBlade: return {WeaponArt::ObsidianRift,1950,7,2,3,"OBSIDIAN RIFT"};
        case EquipmentId::RuneSword: return {WeaponArt::RunicArc,2350,10,3,3,"RUNIC ARC"};
        case EquipmentId::PrimordialBlade: return {WeaponArt::PrimordialNova,2900,16,3,4,"PRIMORDIAL NOVA"};
        default: return {};
    }
}

inline bool weapon_art_ready(const WeaponArtProfile& profile,std::uint32_t now,std::uint32_t last_use) {
    return profile.art!=WeaponArt::None && profile.cooldown_ms>0 &&
           static_cast<std::uint32_t>(now-last_use)>=profile.cooldown_ms;
}

} // namespace mineplus

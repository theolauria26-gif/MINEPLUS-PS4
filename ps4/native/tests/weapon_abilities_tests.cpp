#include "../weapon_abilities.hpp"
#include <cassert>
using namespace mineplus;
int main() {
 const EquipmentId ids[]={EquipmentId::StoneHatchet,EquipmentId::IronSword,EquipmentId::ObsidianBlade,EquipmentId::RuneSword,EquipmentId::PrimordialBlade};
 std::uint16_t previousDamage=0,previousCooldown=0;
 for(const auto id:ids) {
  const auto p=weapon_art(id);
  assert(p.art!=WeaponArt::None&&p.range>=1&&p.durability_cost>=1);
  assert(p.bonus_damage>previousDamage);
  assert(p.cooldown_ms>previousCooldown);
  previousDamage=p.bonus_damage;previousCooldown=p.cooldown_ms;
  assert(!weapon_art_ready(p,1000,0));
  assert(!weapon_art_ready(p,p.cooldown_ms-1,0));
  assert(weapon_art_ready(p,p.cooldown_ms,0));
  assert(weapon_art_ready(p,3000,0xfffffff0u)); // unsigned tick wrap is safe
 }
 assert(weapon_art(EquipmentId::RuneHelm).art==WeaponArt::None);
 assert(!weapon_art_ready(weapon_art(EquipmentId::None),5000,0));
 return 0;
}

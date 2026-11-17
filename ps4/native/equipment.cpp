#include "equipment.hpp"
#include <algorithm>

namespace mineplus {
namespace {
constexpr EquipmentDefinition definitions[] = {
 {EquipmentId::StoneHatchet,EquipmentTier::Primitive,EquipmentSlot::Weapon,{5,0,0,0,0},1,90},
 {EquipmentId::BoneCap,EquipmentTier::Primitive,EquipmentSlot::Head,{0,2,1,0,0},1,80},
 {EquipmentId::HideVest,EquipmentTier::Primitive,EquipmentSlot::Chest,{0,4,2,0,1},1,100},
 {EquipmentId::TrailLeggings,EquipmentTier::Primitive,EquipmentSlot::Legs,{0,2,1,0,0},1,80},
 {EquipmentId::MossBoots,EquipmentTier::Primitive,EquipmentSlot::Boots,{0,1,1,0,1},1,80},
 {EquipmentId::IronSword,EquipmentTier::Iron,EquipmentSlot::Weapon,{12,0,0,0,0},2,180},
 {EquipmentId::IronHelm,EquipmentTier::Iron,EquipmentSlot::Head,{0,5,2,0,1},2,160},
 {EquipmentId::IronChest,EquipmentTier::Iron,EquipmentSlot::Chest,{0,9,4,0,2},2,220},
 {EquipmentId::IronLegs,EquipmentTier::Iron,EquipmentSlot::Legs,{0,6,2,0,1},2,180},
 {EquipmentId::IronBoots,EquipmentTier::Iron,EquipmentSlot::Boots,{0,3,1,0,1},2,160},
 {EquipmentId::ObsidianBlade,EquipmentTier::Obsidian,EquipmentSlot::Weapon,{20,0,0,2,2},3,240},
 {EquipmentId::ObsidianHelm,EquipmentTier::Obsidian,EquipmentSlot::Head,{0,8,3,0,4},3,220},
 {EquipmentId::ObsidianChest,EquipmentTier::Obsidian,EquipmentSlot::Chest,{0,14,6,0,7},3,300},
 {EquipmentId::ObsidianLegs,EquipmentTier::Obsidian,EquipmentSlot::Legs,{0,9,3,0,4},3,250},
 {EquipmentId::ObsidianBoots,EquipmentTier::Obsidian,EquipmentSlot::Boots,{0,5,2,0,3},3,220},
 {EquipmentId::RuneSword,EquipmentTier::Runic,EquipmentSlot::Weapon,{24,0,0,8,3},4,300},
 {EquipmentId::RuneHelm,EquipmentTier::Runic,EquipmentSlot::Head,{0,10,4,5,5},4,280},
 {EquipmentId::RuneChest,EquipmentTier::Runic,EquipmentSlot::Chest,{0,17,8,8,9},4,360},
 {EquipmentId::RuneLegs,EquipmentTier::Runic,EquipmentSlot::Legs,{0,11,4,5,6},4,320},
 {EquipmentId::RuneBoots,EquipmentTier::Runic,EquipmentSlot::Boots,{0,6,3,3,4},4,280},
 {EquipmentId::PrimordialBlade,EquipmentTier::Primordial,EquipmentSlot::Weapon,{34,0,0,14,7},5,420},
 {EquipmentId::PrimordialHelm,EquipmentTier::Primordial,EquipmentSlot::Head,{0,14,6,9,9},5,400},
 {EquipmentId::PrimordialChest,EquipmentTier::Primordial,EquipmentSlot::Chest,{0,23,12,14,15},5,520},
 {EquipmentId::PrimordialLegs,EquipmentTier::Primordial,EquipmentSlot::Legs,{0,15,7,9,10},5,460},
 {EquipmentId::PrimordialBoots,EquipmentTier::Primordial,EquipmentSlot::Boots,{0,9,5,6,8},5,400}
};
// Inventory indices follow main.cpp: Stone=3, Wood=4, Bone=7, IronOre=14,
// Obsidian=15, Ember=16, Crystal=6, IronBar=18, ObsidianCore=19,
// EmberCatalyst=20, AncientRune=21, special loot: Coins=22, Herbs=23, BoneShards=24, DungeonOre=25, CrystalShards=26, RuneFragments=27, EmberCores=28, Relics=29, BossSigils=30.
constexpr EquipmentRecipe recipes[] = {
 {EquipmentId::StoneHatchet,{{{4,3},{3,2},{7,1},{0,0}}},3},
 {EquipmentId::BoneCap,{{{7,4},{4,2},{0,0},{0,0}}},2},
 {EquipmentId::HideVest,{{{7,5},{5,3},{4,2},{0,0}}},3},
 {EquipmentId::TrailLeggings,{{{7,3},{5,2},{4,2},{0,0}}},3},
 {EquipmentId::MossBoots,{{{5,4},{7,2},{4,1},{0,0}}},3},
 {EquipmentId::IronSword,{{{18,3},{14,2},{4,1},{0,0}}},3},
 {EquipmentId::IronHelm,{{{18,4},{14,2},{7,2},{0,0}}},3},
 {EquipmentId::IronChest,{{{18,7},{14,4},{7,3},{0,0}}},3},
 {EquipmentId::IronLegs,{{{18,5},{14,3},{4,2},{0,0}}},3},
 {EquipmentId::IronBoots,{{{18,3},{14,2},{4,2},{0,0}}},3},
 {EquipmentId::ObsidianBlade,{{{15,5},{19,1},{16,3},{14,2}}},4},
 {EquipmentId::ObsidianHelm,{{{15,4},{19,1},{16,2},{7,2}}},4},
 {EquipmentId::ObsidianChest,{{{15,9},{19,2},{16,4},{14,3}}},4},
 {EquipmentId::ObsidianLegs,{{{15,6},{19,1},{16,3},{7,3}}},4},
 {EquipmentId::ObsidianBoots,{{{15,4},{19,1},{16,2},{4,2}}},4},
 {EquipmentId::RuneSword,{{{21,2},{27,3},{28,1},{26,2}}},4},
 {EquipmentId::RuneHelm,{{{21,2},{27,2},{26,2},{6,3}}},4},
 {EquipmentId::RuneChest,{{{21,4},{27,4},{26,3},{6,5}}},4},
 {EquipmentId::RuneLegs,{{{21,3},{27,3},{26,2},{6,3}}},4},
 {EquipmentId::RuneBoots,{{{21,2},{27,2},{26,2},{6,2}}},4},
 {EquipmentId::PrimordialBlade,{{{29,1},{30,1},{27,5},{26,4}}},4},
 {EquipmentId::PrimordialHelm,{{{29,1},{30,1},{27,3},{26,3}}},4},
 {EquipmentId::PrimordialChest,{{{29,2},{30,1},{27,7},{26,6}}},4},
 {EquipmentId::PrimordialLegs,{{{29,1},{30,1},{27,5},{26,4}}},4},
 {EquipmentId::PrimordialBoots,{{{29,1},{30,1},{27,3},{26,3}}},4}
};
const EquipmentDefinition* definition(EquipmentId id) {
 for(const auto& d:definitions)if(d.id==id)return &d;
 return nullptr;
}
const EquipmentRecipe* find_recipe(EquipmentId id) {
 for(const auto& r:recipes)if(r.output==id)return &r;
 return nullptr;
}
}

const EquipmentDefinition* EquipmentCatalog::find(EquipmentId id){return definition(id);}
const EquipmentRecipe* EquipmentCatalog::recipe(EquipmentId id){return find_recipe(id);}

bool EquipmentCatalog::can_craft(EquipmentId id,const int* inventory,std::size_t n){
 const auto* r=find_recipe(id);if(!r||!inventory)return false;
 for(std::size_t i=0;i<r->ingredient_count;++i){
  const auto ingredient=r->ingredients[i];
  if(ingredient.inventory_index>=n||inventory[ingredient.inventory_index]<ingredient.count)return false;
 }
 return true;
}
bool EquipmentCatalog::consume_recipe(EquipmentId id,int* inventory,std::size_t n){
 if(!can_craft(id,inventory,n))return false;
 const auto* r=find_recipe(id);
 for(std::size_t i=0;i<r->ingredient_count;++i){
  const auto ingredient=r->ingredients[i];
  inventory[ingredient.inventory_index]-=ingredient.count;
 }
 return true;
}
EquipmentTotals EquipmentCatalog::calculate(const EquipmentLoadout& loadout){
 EquipmentTotals total{};
 std::array<unsigned,6> sets{};
 for(std::size_t slot=0;slot<loadout.slots.size();++slot){
  const auto* d=definition(loadout.slots[slot]);if(!d||static_cast<std::size_t>(d->slot)!=slot)continue;
  total.damage=static_cast<std::uint16_t>(total.damage+d->stats.damage);
  total.defense=static_cast<std::uint16_t>(total.defense+d->stats.defense);
  total.max_health=static_cast<std::uint16_t>(total.max_health+d->stats.vitality);
  total.arcane=static_cast<std::uint16_t>(total.arcane+d->stats.arcane);
  total.resistance=static_cast<std::uint16_t>(total.resistance+d->stats.resistance);
  if(d->set_id<sets.size())++sets[d->set_id];
 }
 unsigned best=0;
 for(unsigned id=1;id<sets.size();++id)if(sets[id]>best){best=sets[id];total.active_set=static_cast<std::uint8_t>(id);}
 total.set_piece_count=static_cast<std::uint8_t>(best);
 // A partial set does not activate a set identity or bonus.
 if(best<3){total.active_set=0;return total;}
 // Bonuses require three pieces; a complete five-piece set grants a stronger bonus.
 if(best>=3){
  switch(total.active_set){
   case 1: total.defense+=2;break;
   case 2: total.defense+=3;total.max_health+=2;break;
   case 3: total.defense+=4;total.resistance+=5;break;
   case 4: total.defense+=4;total.arcane+=6;total.resistance+=3;break;
   case 5: total.defense+=6;total.arcane+=8;total.resistance+=6;total.max_health+=4;break;
  }
  if(best==5){total.max_health+=3;total.resistance+=2;}
 }
 return total;
}

bool EquipmentCatalog::craft(EquipmentState& state,EquipmentId id,int* inventory,std::size_t n){
 const auto raw=static_cast<std::size_t>(id);
 const auto* d=definition(id);
 if(!d||raw>=state.owned.size()||(state.owned[raw]&&state.durability[raw]>0))return false;
 if(!consume_recipe(id,inventory,n))return false;
 state.owned[raw]=1;
 state.durability[raw]=d->durability;
 return true;
}
bool EquipmentCatalog::equip(EquipmentState& state,EquipmentId id){
 const auto raw=static_cast<std::size_t>(id);
 const auto* d=definition(id);
 if(!d||raw>=state.owned.size()||!state.owned[raw]||state.durability[raw]==0)return false;
 state.equipped[static_cast<std::size_t>(d->slot)]=id;
 return true;
}
EquipmentTotals EquipmentCatalog::calculate(const EquipmentState& state){
 EquipmentLoadout loadout{};
 for(std::size_t slot=0;slot<loadout.slots.size();++slot){
  const auto id=state.equipped[slot];
  const auto raw=static_cast<std::size_t>(id);
  if(raw<state.owned.size()&&state.owned[raw]&&state.durability[raw]>0)loadout.slots[slot]=id;
 }
 return calculate(loadout);
}
void EquipmentCatalog::wear(EquipmentState& state,EquipmentSlot slot,std::uint16_t amount){
 const auto index=static_cast<std::size_t>(slot);
 if(index>=state.equipped.size()||amount==0)return;
 const auto id=state.equipped[index];
 const auto raw=static_cast<std::size_t>(id);
 if(raw>=state.durability.size()||!state.owned[raw])return;
 state.durability[raw]=state.durability[raw]>amount?static_cast<std::uint16_t>(state.durability[raw]-amount):0;
}
std::uint16_t EquipmentCatalog::mitigate_damage(std::uint16_t incoming,const EquipmentTotals& totals,bool elemental){
 if(incoming==0)return 0;
 const unsigned mitigation=static_cast<unsigned>(totals.defense)/8u+
                           (elemental?static_cast<unsigned>(totals.resistance)/10u:0u);
 return static_cast<std::uint16_t>(incoming>mitigation?incoming-mitigation:1u);
}
} // namespace mineplus

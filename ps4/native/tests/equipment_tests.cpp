#include "../equipment.hpp"
#include <cassert>
#include <iostream>

using namespace mineplus;

int main(){
 int inventory[31]{};
 inventory[4]=10;inventory[3]=10;inventory[7]=5;
 assert(EquipmentCatalog::can_craft(EquipmentId::StoneHatchet,inventory,31));
 assert(EquipmentCatalog::consume_recipe(EquipmentId::StoneHatchet,inventory,31));
 assert(inventory[4]==7&&inventory[3]==8&&inventory[7]==4);
 assert(!EquipmentCatalog::consume_recipe(EquipmentId::StoneHatchet,inventory,2));
 assert(EquipmentCatalog::find(EquipmentId::PrimordialBlade)!=nullptr);
 assert(EquipmentCatalog::recipe(EquipmentId::PrimordialChest)!=nullptr);
 assert(EquipmentCatalog::find(EquipmentId::Count)==nullptr);
 for(unsigned id=1;id<static_cast<unsigned>(EquipmentId::Count);++id){
  const auto equipment=static_cast<EquipmentId>(id);
  assert(EquipmentCatalog::find(equipment)!=nullptr);
  const auto* recipe=EquipmentCatalog::recipe(equipment);
  assert(recipe!=nullptr&&recipe->ingredient_count>0&&recipe->ingredient_count<=4);
  for(unsigned i=0;i<recipe->ingredient_count;++i)assert(recipe->ingredients[i].inventory_index<31);
 }
 int insufficient[31]{};insufficient[4]=2;insufficient[3]=1;
 const int before[31]={};
 assert(!EquipmentCatalog::consume_recipe(EquipmentId::StoneHatchet,insufficient,31));
 for(int i=0;i<31;++i)assert(insufficient[i]==before[i]);

 EquipmentLoadout loadout{};
 loadout.slots[0]=EquipmentId::RuneSword;
 loadout.slots[1]=EquipmentId::RuneHelm;
 loadout.slots[2]=EquipmentId::RuneChest;
 auto totals=EquipmentCatalog::calculate(loadout);
 assert(totals.damage==24);
 assert(totals.defense==31);
 assert(totals.active_set==4&&totals.set_piece_count==3);
 assert(totals.arcane==27);
 assert(totals.resistance==20);

 loadout.slots[3]=EquipmentId::RuneLegs;
 loadout.slots[4]=EquipmentId::RuneBoots;
 totals=EquipmentCatalog::calculate(loadout);
 assert(totals.set_piece_count==5);
 assert(totals.max_health>10);
 assert(totals.resistance>=26);

 EquipmentLoadout invalid{};
 invalid.slots[0]=EquipmentId::RuneHelm; // wrong slot: ignored, never grants weapon stats
 totals=EquipmentCatalog::calculate(invalid);
 assert(totals.damage==0&&totals.defense==0);

 EquipmentLoadout mixed{};
 mixed.slots[0]=EquipmentId::IronSword;
 mixed.slots[1]=EquipmentId::RuneHelm;
 mixed.slots[2]=EquipmentId::ObsidianChest;
 totals=EquipmentCatalog::calculate(mixed);
 assert(totals.active_set==0);
 EquipmentState state{};
 int forgeStock[31]{};forgeStock[18]=3;forgeStock[14]=2;forgeStock[4]=1;
 assert(EquipmentCatalog::craft(state,EquipmentId::IronSword,forgeStock,31));
 assert(state.owned[static_cast<unsigned>(EquipmentId::IronSword)]==1);
 assert(!EquipmentCatalog::craft(state,EquipmentId::IronSword,forgeStock,31));
 assert(EquipmentCatalog::equip(state,EquipmentId::IronSword));
 auto equippedTotals=EquipmentCatalog::calculate(state);
 assert(equippedTotals.damage==12);
 const auto swordIndex=static_cast<unsigned>(EquipmentId::IronSword);
 assert(state.durability[swordIndex]>1);
 EquipmentCatalog::wear(state,EquipmentSlot::Weapon,static_cast<std::uint16_t>(state.durability[swordIndex]-1));
 assert(EquipmentCatalog::calculate(state).damage==12);
 EquipmentCatalog::wear(state,EquipmentSlot::Weapon,1);
 assert(state.durability[swordIndex]==0);
 assert(EquipmentCatalog::calculate(state).damage==0);
 EquipmentTotals armor{};armor.defense=16;armor.resistance=20;
 assert(EquipmentCatalog::mitigate_damage(5,armor,false)==3);
 assert(EquipmentCatalog::mitigate_damage(5,armor,true)==1);
 assert(EquipmentCatalog::mitigate_damage(1,armor,true)==1);
 assert(EquipmentCatalog::mitigate_damage(0,armor,true)==0);
 std::cout<<"Equipment catalog tests passed\n";
}

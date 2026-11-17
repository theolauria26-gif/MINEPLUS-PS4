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

 EquipmentLoadout loadout{};
 loadout.slots[0]=EquipmentId::RuneSword;
 loadout.slots[1]=EquipmentId::RuneHelm;
 loadout.slots[2]=EquipmentId::RuneChest;
 auto totals=EquipmentCatalog::calculate(loadout);
 assert(totals.damage==24);
 assert(totals.defense==27);
 assert(totals.active_set==4&&totals.set_piece_count==3);
 assert(totals.arcane==21);
 assert(totals.resistance==17);

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
 std::cout<<"Equipment catalog tests passed\n";
}

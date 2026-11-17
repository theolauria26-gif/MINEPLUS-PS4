# Equipment progression

The equipment catalog defines five progression tiers and five loadout slots:

1. Primitive — Stone Hatchet and bone/hide/moss protection.
2. Iron — a balanced metal weapon and four armor pieces.
3. Obsidian — high physical defense with ember/obsidian resistance.
4. Runic — arcane power and rune resistance.
5. Primordial — the strongest mixed defense, vitality and arcane bonuses.

There are 25 catalog entries. Every item has a stable ID, tier, slot, combat stats, set ID, recipe and durability budget. Recipes use the existing 31-slot gameplay inventory indices. Crafting validates all ingredients before consuming any and currently allows one owned copy of each catalog item.

## Live gameplay integration

- Open the inventory with E / Triangle, then switch to the equipment page with V / left-stick click.
- Navigate the 25-item grid with left/right or controller shoulders.
- Enter / controller A crafts the selected item.
- U / controller right-stick equips an owned item. Newly crafted gear automatically fills an empty matching slot.
- Five slots are active: weapon, head, chest, legs and boots.
- Three matching set pieces activate the set bonus; five pieces add the completion bonus.
- Weapon damage changes creature hit damage. Armor defense and elemental resistance mitigate incoming hits.
- Weapon durability decreases when striking creatures; armor durability decreases when the player is hit. Broken equipment remains owned but contributes no stats; crafting the same item again consumes its recipe and reforges it to full durability. A dedicated repair station is still a future feature.

## Save compatibility

Save version 10 appends equipment ownership flags, equipped slot IDs and per-item durability after the version-9 loot payload. The loader still accepts versions 1–9; those saves start with an empty equipment state. Version-10 equipment payloads are validated before applying them to the live state. The previous save header layout and world/inventory payload remain unchanged.

## Remaining work

This system is now connected to the inventory UI, crafting, equipped stats, combat and save/load. Equipment-specific attack animations, repair stations, richer weapon moves, visual armor models and additional accessory slots are not implemented yet. The current renderer remains the existing pseudo-3D SDL2 renderer; this catalog does not imply a GPU voxel renderer.

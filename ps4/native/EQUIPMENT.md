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

## Repair and maintenance

The equipment catalog now supports full-durability repairs without changing the save format. Repairing requires an owned, damaged item and consumes a tier-specific material pair atomically: primitive gear uses wood/bone, iron uses iron bars/bone, obsidian uses obsidian/ember, runic uses ancient runes/rune fragments, and primordial gear uses ancient relics/boss sigils. Full-durability items, unowned items, and repairs with insufficient materials are rejected without consumption. The repair action is available on the equipment inventory page: press **R** on keyboard or **X** on the controller while the damaged item is selected. Successful repairs save immediately. This is field maintenance rather than a placed forge station; a physical forge station, animations and repair feedback are still future polish.

## Weapon arts

Equipped weapons now provide one tier-specific special attack in live combat. Press **G** on keyboard or click **R3** (right-stick button) on the controller while outside the inventory. The move hits creatures in a straight line in front of the player, within its profile range; it only consumes durability and starts its cooldown when at least one creature is hit.

| Weapon | Art | Range | Bonus damage | Cooldown | Durability |
|---|---|---:|---:|---:|---:|
| Stone Hatchet | Heavy Strike | 1 | +2 | 1.3 s | 2 |
| Iron Sword | Iron Cleave | 2 | +4 | 1.6 s | 2 |
| Obsidian Blade | Obsidian Rift | 2 | +7 | 1.95 s | 3 |
| Rune Sword | Runic Arc | 3 | +10 | 2.35 s | 3 |
| Primordial Blade | Primordial Nova | 3 | +16 | 2.9 s | 4 |

The hit also produces a brief slash flash at the crosshair. Weapon arts are implemented as combat rules and a 2D feedback effect in the current SDL2 pseudo-3D renderer; they are not skeletal animations or GPU particle effects. Cooldowns are session timers and reset when the game restarts; weapon durability remains persisted by save version 10.

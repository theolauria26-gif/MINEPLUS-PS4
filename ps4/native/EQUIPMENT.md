# Equipment progression

The equipment catalog defines five progression tiers and five loadout slots:

1. Primitive — Stone Hatchet and bone/hide/moss protection.
2. Iron — a balanced metal weapon and four armor pieces.
3. Obsidian — high physical defense with ember/obsidian resistance.
4. Runic — arcane power and rune resistance.
5. Primordial — the strongest mixed defense, vitality and arcane bonuses.

Each definition has stable numeric identity, tier, slot, combat stats, set ID and durability budget. Recipes use the existing 31-slot gameplay inventory indices; crafting validates every ingredient before consuming any, so failed crafts never partially spend materials.

Three matching pieces activate a tier bonus. A complete five-piece set receives an additional completion bonus. Wrong-slot equipment is ignored by stat calculation, and unknown IDs are rejected.

This is currently a tested CPU gameplay-data module. It is not yet wired to the live inventory UI, player combat, durability wear, equipment persistence or save format. Existing save version 9 is deliberately unchanged until the complete equipped-item ownership and migration design is integrated atomically.

# Deterministic dungeon loot

`DungeonLootGenerator` creates room rewards from the world seed, room ID and room type. Identical inputs always produce identical drops, so a room cannot reroll rewards when revisited.

- Combat rooms: coins, bone shards, herbs and occasional iron.
- Treasure rooms: larger coin/ore rewards, crystals, rune fragments and rare ember cores.
- Puzzle rooms: rune fragments and crystals, with a small healing reward chance.
- Boss rooms: guaranteed Boss Sigil plus a weighted selection of relics, ember cores and crystals.
- Start and exit rooms intentionally produce no loot.

The API returns stable item IDs and stack counts. It does not yet connect these drops to the live inventory, chest interaction, or saved dungeon state; those are separate gameplay integration tasks.

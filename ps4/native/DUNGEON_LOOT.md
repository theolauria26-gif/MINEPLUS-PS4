# Deterministic dungeon loot

`DungeonLootGenerator` creates room rewards from the world seed, room ID and room type. Identical inputs always produce identical drops, so a room cannot reroll rewards when revisited.

- Combat rooms: coins, bone shards, herbs and occasional iron.
- Treasure rooms: larger coin/ore rewards, crystals, rune fragments and rare ember cores.
- Puzzle rooms: rune fragments and crystals, with a small healing reward chance.
- Boss rooms: guaranteed Boss Sigil plus a weighted selection of relics, ember cores and crystals.
- Start and exit rooms intentionally produce no loot.

The API returns stable item IDs and stack counts. It does not yet connect these drops to the live inventory, chest interaction, or saved dungeon state; those are separate gameplay integration tasks.


## Persistent chest state

Dungeon chest contents and their opened/closed state are now part of `WorldArchive` save format version 3. Each chest is keyed by the stable room ID and stores at most 32 distinct item stacks. A world save supports up to 256 dungeon chests.

- `DungeonLootGenerator::generate_chest` turns a room into a stable unopened chest state; `WorldRuntime::set_dungeon_chest` and `get_dungeon_chest` store and retrieve it.
- Invalid item IDs, zero/oversized stacks, duplicate room IDs and duplicate item stacks are rejected.
- The save codec uses explicit little-endian fields and includes chest data in the existing checksum.
- Version 1 and version 2 archives remain readable; they restore with an empty chest list.
- Decode validates the complete archive before replacing the runtime's chest state.

This adds persistent storage to the streamed-world runtime. The current legacy SDL gameplay loop still uses its separate version-8 world save and does not yet spawn dungeon chests or transfer loot into the player's live inventory.

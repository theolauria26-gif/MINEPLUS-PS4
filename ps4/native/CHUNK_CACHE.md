# Chunk cache and streaming layer

`chunk_cache.hpp/.cpp` adds a bounded, fixed-memory LRU cache for procedural chunks.

## Runtime contract

- Chunk dimensions: 16 x 16 tiles.
- Cache capacity: 81 chunks (9 x 9), approximately 384 KiB for generated surface, structure and flag arrays.
- `update_center(x, y, 4)` loads the nearest 9 x 9 chunk neighborhood center-out. No heap allocation occurs when the cache is used.
- `get(x, y, sample)` returns a cached/generated surface sample.
- `set_surface(x, y, tile)` records a player edit in a bounded 2,048-entry delta table and updates the resident chunk.
- Evicted chunks are regenerated deterministically from the same seed; saved-in-memory deltas are replayed on reload.
- If the delta table is full, edits fail explicitly rather than being silently discarded.

## Important integration status

This module is compiled into the PS4 EBOOT and covered by portable regression tests. The existing game loop still uses the legacy 2048 x 2048 resident world and its versioned save file. The cache is the next world-storage backend, but player movement, renderer, combat, entity simulation and save/load have not yet been migrated to it. Therefore this commit does **not** claim playable infinite-world streaming.

## PS4 performance rules

- Keep chunk generation free of SDL and GPU calls.
- Load the player's chunk first and expand the radius incrementally if frame time becomes an issue.
- Do not increase cache capacity without measuring memory use on the console.
- Persist the delta table atomically before evicting or closing; the current in-memory table is not yet a durable save format.
- A future worker may generate immutable `ChunkData`; only the main thread may upload/render resources.

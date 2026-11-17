# Renderer status and voxel integration plan

## Current PS4 presentation path

- The live game uses SDL2's accelerated 2D renderer when available, with a software fallback.
- `draw_world()` performs a first-person height-field/ray-march over the resident 2D tile map. It projects terrain columns and creatures into a pseudo-3D scene.
- Material appearance is currently generated procedurally in `draw_material_texture()` and `tile_color()`; there is no shipped Hytale texture atlas or imported third-party art.
- The new stone/darkstone surface pattern uses staggered masonry joints; wood uses directional grain; obsidian and iron bars have dedicated highlights.
- The CPU voxel pipeline (`VoxelMesher`, `VoxelMeshVertexBuilder`, and `ProceduralVoxelChunkBuilder`) is not yet submitted to a GPU draw path.

## Overworld surface generation

- The deterministic PCG surface sampler now carves meandering river channels through moist temperate land, with sandy banks that transition into plains, forests and highlands.
- River fields use global coordinates and the world seed, so neighboring chunks and reloads reproduce the same channels. The biome identity remains intact for climate and later surface ecology.
- Surface voxel sampling reports river water as liquid and river banks as solid sand. This is CPU world-generation data consumed by the current surface tile map; it is not a new GPU water renderer.
- Inland lakes now use seed-derived macro-cell basins with varied radii and anisotropic shorelines; water lowers the local surface profile and nearby eligible terrain transitions to sand. Lake water is exposed as liquid voxel data and regression-tested as multi-sample pools across deterministic world instances. These remain CPU-generated surface tiles, not animated GPU water or simulated fluid.
- Surface elevation now combines broad ridged mountain-chain fields with a separate winding valley field. Mountain uplift influences both biome selection and column height, while valley carving lowers connected corridors; regression tests verify deterministic samples, highland presence and a meaningful elevation range. This shapes the existing height-field terrain and does not add true 3D cliff meshes or erosion simulation.
- Warm coastal shelves now transition into sandy beaches before the ocean boundary; cold regions retain their snow/ice surface and desert identity is kept separate. The coastline regression scan checks repeatability and solid sand voxel samples. Beaches use the current surface height-field, not a separate shoreline mesh or wave simulation.
- Forest biome trees now form broad deterministic groves with clearings; moist plains can contain sparse tree clusters. Desert, ocean, volcanic and crystal-waste regions do not receive these trees. Placement is derived from seed/global coordinates and is covered by portable PCG tests.
- Natural tree crowns now vary deterministically in width, depth and edge silhouette, producing compact, broad and wind-shaped canopy profiles within the same forest groves. The trunk remains the collision point and leaves remain walk-through; this is a tile-map silhouette variation, not skeletal foliage or wind simulation.
- Grass material textures include sparse, deterministic wildflower flecks in several muted colors. They are visual pixels in the existing SDL material pass, not collectible flower entities or new saved tile types.
- Surface landmark generation now includes abandoned camps as a fifth deterministic structure family. The rasterized camp uses a dark-stone perimeter, timber crossbeams, moss patches and an ember hearth; its appearance is static in the current height-field renderer.
- Surface landmark generation now also includes ruined frontier watch-posts as a sixth deterministic family. Their footprint combines a broken dark-stone ring, timber lookout braces, an ancient-rune approach, crystal markers, moss and a central chest tile. These are static tile-map landmarks, not multi-level enterable buildings or animated towers.
- Nearby surface landmarks are now joined by deterministic nearest-neighbor dirt tracks. The generator selects stable anchor pairs within a bounded radius, draws a narrow sand-colored route between their clear perimeters, and leaves lake/lava crossings untouched. Roads are static terrain tiles in the current height-field renderer; there are no dynamic traffic, bridges, or pathfinding agents yet.
- The surface renderer now places a dedicated leafy canopy tile around deterministic forest trunks. Canopy foliage is walk-through, has its own green pixel texture and raised height-field profile, and can be harvested for moss. This is still the existing SDL height-field renderer, not GPU foliage or wind animation.
- Initial creature placement now searches for valid land clearings, avoids liquid/solid landmark tiles and the spawn/boss arenas, then assigns creature families from the local biome (woodland/plains, desert/tundra, or highland/hostile regions). Failed bounded searches leave that creature slot inactive instead of embedding it in terrain.

## Requirements before true voxel rendering

1. Confirm the exact SDL2/OpenOrbis renderer capabilities and available GPU APIs in the pinned PS4 toolchain.
2. Add a PS4-compatible shader/texture upload path only after its API and linkage are verified.
3. Upload greedy chunk meshes and material atlas data, with explicit vertex/index budgets.
4. Add opaque and translucent passes separately; liquids are intentionally omitted from the current opaque chunk builder.
5. Integrate chunk visibility, camera transforms, depth testing, and rebuilds after player edits.
6. Keep the existing renderer as a fallback until the new path passes device testing.

## Honesty boundary

A successful CI PKG proves that the native application compiles and packages. It does not prove that voxel chunks are rendered in-game or that the package has been tested on a physical PS4. The current material-pattern improvements affect the existing height-field renderer; the CPU voxel mesh remains a separate pipeline.

## OpenOrbis GPU API audit

The pinned OpenOrbis toolchain source tree was inspected directly:

- `include/orbis/GnmDriver.h` declares low-level GNM entry points including `sceGnmDrawIndex`, `sceGnmDrawIndexAuto`, `sceGnmSetVsShader`, `sceGnmSetPsShader`, `sceGnmSubmitCommandBuffers`, and `sceGnmSubmitDone`.
- The declarations confirm that a native GNM draw/submit route exists in the toolchain. They do not, by themselves, provide a complete renderer: the header contains many untyped/empty declarations, and the project does not yet own command-buffer allocation, GPU memory mapping, shader binaries, pipeline state, render targets, synchronization, or presentation.
- The OpenOrbis SDL2 sample confirms SDL2 is packaged and linked as `-lSDL2`; it is a 2D SDL sample, not evidence of a voxel GPU mesh renderer.
- The current MINEPLUS Makefile links `-lSDL2` and PS4 system libraries, but not a GNM rendering module. No GNM calls are made by the game.

### Safe implementation sequence

1. Obtain and inspect the exact v0.5.4 SDK headers and import libraries used by CI, including GNM and kernel memory APIs; confirm exported symbols and signatures rather than relying on placeholder declarations.
2. Build a tiny isolated GNM triangle proof-of-concept with a known shader binary, command buffers, GPU-visible allocations and a video-out flip. Keep it outside the gameplay renderer until it compiles and is device-validated.
3. Only then add voxel vertex/index uploads, depth-tested opaque geometry, atlas sampling, and separate translucent passes.
4. Retain SDL2 as the fallback and UI path. Do not link speculative GNM calls into the shipping game.

This audit narrows the next task: the toolchain exposes low-level GNM draw/submit symbols, but a safe GPU renderer still needs a verified memory/command/shader/presentation foundation. A CI package alone cannot validate GPU execution on PS4 hardware.


## Surface wildlife ecology

- Surface wildlife now uses a standalone deterministic species catalog for the twelve existing creature IDs. The catalog gives each species a name, biome affinity, climate bounds and behavior role (grazer, skittish, predator or territorial).
- Initial placement now asks the ecology catalog to select a species compatible with the sampled biome, moisture, temperature and land surface. Ocean, water and sandy shoreline cells are rejected; if no species fits the sampled climate, the bounded spawn attempt is retried.
- Creature movement reads the catalog behavior role: grazers and skittish wildlife flee when approached, while predators and territorial species pursue. This reuses the existing simple tile-step movement; it is not advanced pathfinding, pack coordination, animation or a new rendered model.
- Portable tests cover all twelve species IDs, biome/climate eligibility, invalid surfaces, deterministic selection and behavior roles.


## Frontier settlements

- Added deterministic settlement site selection on temperate Plains/Forest clearings with moisture and temperature bounds and a five-point height/surface check.
- Each accepted site has one of four five-building layouts and a stable resident-count target (5–7). The current gameplay integration rasterizes five distinct house/workshop footprints, a central hearth and walkable doorways after the road pass.
- Each accepted settlement now populates 5–7 named residents at building entrances and around the central square. Five roles (elder, smith, herbalist, scout and warden) have distinct pseudo-3D silhouettes, short deterministic patrol schedules and three rotating dialogue lines each. Keyboard T or controller L3 talks to the nearest resident within two tiles.
- Resident movement is bounded to a two-tile patrol around each home and checks solid/liquid terrain plus other residents before stepping. Actors and dialogue are regenerated deterministically at world initialization; per-NPC schedules and dialogue stages are not persisted.
- Residents now offer role-based trades for coins: the elder exchanges runic relics, the smith offers metalwork, the herbalist trades natural materials, the scout offers exploration resources and the warden supplies frontier materials. Purchases validate coin balance and inventory capacity atomically; successful trades save immediately. Keyboard B opens trading, arrows select and Enter buys. On controller, approach a resident and press X to open/buy; D-pad selects and Circle closes. Offers are currently unlimited rather than depleting merchant stock.
- Five role-specific surface quests are now available through the same resident interaction (keyboard T or controller L3): the elder requests Rune Fragments, the smith Iron Bars, the herbalist Herbs, the scout Crystal Shards and the warden Bone Shards. Gathering matching materials while a quest is active advances its counter; returning to that resident claims the reward. Quest state and progress are appended to save version 12, with older versions defaulting to an empty quest journal.
- Timed overworld events rotate deterministically during active play: Caravan Market (25% trade discount), Harvest Festival (+1 bonus for frontier gathering materials), Wildlife Migration (faster wildlife movement), Rune Storm (bonus runic/crystal materials) and Predator Warning (faster creature attacks). Events last 60 seconds in a 180-second cycle and pause with the game/inventory; the schedule is session-time based and is not persisted.
- The SDL height-field renderer draws stylized billboard-like figures, not skeletal 3D character models. Dialogue stage and patrol schedules remain regenerated at world initialization; quest completion is persisted.

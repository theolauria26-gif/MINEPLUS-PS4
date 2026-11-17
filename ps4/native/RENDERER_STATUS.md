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

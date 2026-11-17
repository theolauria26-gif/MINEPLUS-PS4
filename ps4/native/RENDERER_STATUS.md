# Renderer status and voxel integration plan

## Current PS4 presentation path

- The live game uses SDL2's accelerated 2D renderer when available, with a software fallback.
- `draw_world()` performs a first-person height-field/ray-march over the resident 2D tile map. It projects terrain columns and creatures into a pseudo-3D scene.
- Material appearance is currently generated procedurally in `draw_material_texture()` and `tile_color()`; there is no shipped Hytale texture atlas or imported third-party art.
- The new stone/darkstone surface pattern uses staggered masonry joints; wood uses directional grain; obsidian and iron bars have dedicated highlights.
- The CPU voxel pipeline (`VoxelMesher`, `VoxelMeshVertexBuilder`, and `ProceduralVoxelChunkBuilder`) is not yet submitted to a GPU draw path.

## Requirements before true voxel rendering

1. Confirm the exact SDL2/OpenOrbis renderer capabilities and available GPU APIs in the pinned PS4 toolchain.
2. Add a PS4-compatible shader/texture upload path only after its API and linkage are verified.
3. Upload greedy chunk meshes and material atlas data, with explicit vertex/index budgets.
4. Add opaque and translucent passes separately; liquids are intentionally omitted from the current opaque chunk builder.
5. Integrate chunk visibility, camera transforms, depth testing, and rebuilds after player edits.
6. Keep the existing renderer as a fallback until the new path passes device testing.

## Honesty boundary

A successful CI PKG proves that the native application compiles and packages. It does not prove that voxel chunks are rendered in-game or that the package has been tested on a physical PS4. The current material-pattern improvements affect the existing height-field renderer; the CPU voxel mesh remains a separate pipeline.

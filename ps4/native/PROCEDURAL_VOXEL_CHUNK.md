# Procedural Voxel Chunk Builder

`ProceduralVoxelChunkBuilder` materializes `PcgWorld::sample_voxel` into a 16x16x128 `VoxelChunk`.

- Samples global coordinates and fills the one-voxel halo for seam-aware greedy meshing.
- Builds into a temporary chunk and replaces output only on success.
- Rejects origins whose halo would overflow signed 32-bit coordinates.
- Preserves solid materials, including primordial-cave darkstone and crystal seams.
- Omits liquids because the current chunk format and opaque mesher have no translucent liquid pass.

This is a CPU generation bridge, not renderer integration, and does not yet replace the resident world path in `main.cpp`.

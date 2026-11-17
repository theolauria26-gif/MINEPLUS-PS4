# Procedural Voxel Chunk Builder

`ProceduralVoxelChunkBuilder` materializes the procedural 3D world into a 16x16x128 `VoxelChunk`.

- The `PcgWorld` overload samples deterministic global coordinates directly.
- The `WorldRuntime` overload streams the required 3x3 neighborhood and applies player voxel/surface edits before meshing.
- Both fill the one-voxel horizontal halo for seam-aware greedy meshing.
- Both build into a temporary chunk and replace output only on success; coordinate overflow is rejected.
- Solid materials include primordial-cave darkstone and crystal seams.
- Liquids are omitted because the current chunk format and opaque mesher have no translucent liquid pass.

This is a CPU generation bridge, not renderer integration. It does not yet replace the resident world path in `main.cpp`.

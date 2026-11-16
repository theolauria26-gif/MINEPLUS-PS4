# Voxel mesh vertex contract

`voxel_mesh_vertices` converts the greedy mesher's chunk-local rectangles into
four CPU vertices per quad. Each vertex carries a position, outward normal,
material ID, and tile-local UV measured in voxel units. Winding is CCW when
viewed from outside, using the six tested face bases.

This is deliberately a renderer-neutral intermediate: it does not create a
GPU buffer, shader, texture, or draw call. The future shader must use the
material ID to select the atlas cell and repeat `fract(tile_uv)` inside that
cell; directly scaling atlas UVs across a merged quad would stretch the tile.
The half-texel atlas bounds remain the responsibility of
`material_atlas_region()`.

The builder rejects air/reserved material IDs, invalid faces, zero-area quads,
and vertex-budget overflow, clearing partial output on failure.

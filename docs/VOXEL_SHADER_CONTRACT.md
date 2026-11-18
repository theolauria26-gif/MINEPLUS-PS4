# MINEPLUS voxel shader contract (CPU-side specification)

This document specifies the data expected by a future GPU shader. It is not a compiled shader, GNM pipeline, or claim that voxel meshes are currently rendered in 3D.

## Vertex stream

The staging record is `mineplus::VoxelGpuVertex`, with a fixed stride of 36 bytes:

| Attribute | Type | Byte offset | Meaning |
|---|---:|---:|---|
| position | float3 | 0 | Chunk-local voxel position |
| uv | float2 | 12 | Tile-local UV in voxel units; values can exceed 1 for greedy quads |
| normal | float3 | 20 | Axis-aligned outward normal |
| material | uint32 | 32 | Material ID, 1 through 23 |

The CPU type has compile-time standard-layout, trivial-copy and offset/stride assertions. A backend must still confirm its actual vertex-fetch format and alignment requirements before binding this stream.

## Material atlas

- Atlas dimensions: 256 × 128 RGBA8.
- Grid: 8 columns × 4 rows.
- Cell dimensions: 32 × 32 pixels.
- Material IDs are contiguous, row-major, beginning at 1.
- For ID `m`, the zero-based cell index is `m - 1`; cell coordinates are `((m - 1) % 8, (m - 1) / 8)`.
- Only IDs 1 through 23 are currently assigned. ID 0 is air and must not be submitted as a surface material.

The fragment stage should repeat the tile-local UV within each material cell, then map the fractional coordinate into that cell's normalized atlas rectangle. The rectangle should use the existing half-texel inset from `material_atlas_region()` to avoid sampling adjacent cells. `material_atlas_resolve_uv()` is the CPU reference implementation: it uses floor-based wrapping (including negative UVs), rejects non-finite coordinates and unknown material IDs, and clears outputs on failure. A shader implementation should match this behavior. Texture filtering and mip behavior must be chosen with pixel-art atlas bleeding in mind; no mip chain or sampler state is currently implemented.

## Geometry and rasterization

- Each greedy quad emits four vertices and six uint32 indices.
- Index order per quad is `0,1,2, 0,2,3` relative to its four-vertex base.
- Tests calculate both triangle cross products and require them to agree with the outward axial normal.
- Face normals are one of the six signed cardinal axes.
- Back-face culling may be enabled only after the backend's front-face convention is explicitly matched to this winding test.

## Suggested shader behavior (not yet implemented)

1. Vertex stage transforms chunk-local positions by chunk/world and camera matrices.
2. It passes tile-local UV, flat material ID, and normal to the fragment stage.
3. Fragment stage resolves the row-major atlas cell from the material ID, repeats UV with fractional coordinates, applies the half-texel inset, samples RGBA8, and applies directional lighting using the interpolated normal.
4. Alpha is currently opaque for defined atlas materials; transparent materials need a separate material/render-state policy rather than silently changing this contract.

## Explicit non-goals / missing GPU work

This contract does not supply shader source or binaries, GNM resource descriptors, GPU memory allocation, texture upload, depth state, command buffer construction, synchronization, render-target setup, or VideoOut presentation. Those must be implemented and validated against the exact PS4 SDK/toolchain and preferably hardware before switching away from the current SDL2 renderer.


## Atlas sampling reference tests

Portable tests cover row-major cells, half-texel bounds, integer-repeat boundaries, UVs beyond one tile, negative UV wrapping, and rejection of air/non-finite coordinates. These tests validate the CPU reference contract only; they do not execute a GPU shader.

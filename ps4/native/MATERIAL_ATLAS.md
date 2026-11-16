# Material atlas contract (PS4 native)

This module defines a stable CPU-side mapping from gameplay material IDs to
normalized texture coordinates. It is a renderer integration primitive; it
does **not** create an image atlas, upload a texture, or change the current
SDL2 column renderer.

## Layout
- Atlas dimensions: 128 × 64 pixels.
- Cell dimensions: 16 × 16 pixels.
- Grid: 8 columns × 4 rows (32 available cells).
- Material ID 0 is air and has no region.
- Current solid material IDs 1..21 map row-major to cells 0..20.
- Cells 21..31 are reserved for future materials.

The mapping uses half-texel-inset UV bounds. The eventual GPU sampler should
use nearest-neighbor filtering for crisp pixel-art textures. A linear sampler
or mipmaps require a padded/guttered atlas design before being enabled, or
neighboring cells may bleed at tile boundaries.

## API
material_atlas_region(material, output) clears output first, returns false
for air or unknown IDs, and otherwise fills normalized UV bounds and integer
cell coordinates. The material IDs intentionally match the native gameplay
Tile enum; update the range and tests together whenever that enum changes.

## Integration status
The atlas image, texture upload, GPU sampler state, per-face UV emission, and
runtime renderer integration are not part of this module yet. The current
voxel mesher emits material IDs but remains separate from gameplay rendering.

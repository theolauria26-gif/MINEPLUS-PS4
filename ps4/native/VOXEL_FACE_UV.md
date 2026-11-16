# Voxel face UV orientation

`voxel_face_uv_basis` defines a stable tangent basis for every face emitted
by the greedy voxel mesher. The two signed axes span the face; their cross
product points along the face's outward normal. This gives the future vertex
builder a deterministic orientation instead of guessing based on face order.

| Face | U axis | V axis | U sign | V sign |
|---|---|---|---:|---:|
| -X | Y | Z | -1 | +1 |
| +X | Y | Z | +1 | +1 |
| -Y | Z | X | -1 | +1 |
| +Y | Z | X | +1 | +1 |
| -Z | X | Y | -1 | +1 |
| +Z | X | Y | +1 | +1 |

The cross-product test verifies outward winding for all six faces. This module
does not emit GPU vertices or UV coordinates and does not solve atlas tile
repetition; the renderer must use the basis with a tile-aware shader or
per-face geometry strategy. In particular, simply stretching one 16×16 atlas
cell across a large greedy quad would stretch the texture rather than repeat
it.

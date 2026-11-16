# Voxel mesher (PS4 native)

The native renderer currently uses the existing SDL2 column renderer. The voxel
mesher is a separate, portable CPU module: it is **not yet wired into gameplay
render submission**. Keeping that distinction explicit prevents a successful
unit test or PKG build from being mistaken for a completed 3D renderer.

## Data contract

- Chunk interior: 16 × 16 × 128 voxels.
- One-voxel halo on all six sides, addressable at -1 and SIZE on each axis.
- Material ID 0 is air; non-zero IDs are opaque solid materials.
- The halo is populated by the chunk owner from neighboring chunks before
  meshing. A non-air halo voxel suppresses the matching boundary face.
- Greedy merging combines coplanar exposed faces only when their material IDs
  match. Opaque interfaces between two non-air materials are culled.
- Output coordinates and dimensions are chunk-local voxel units.

## Bounded build

`VoxelMesher::build(chunk, output, max_quads)` clears output before work. It
returns false and leaves output empty if the configured quad budget would be
exceeded. A mesh containing exactly `max_quads` quads is valid. Callers should
choose a budget from measured memory and frame-time limits rather than assuming
the default is appropriate for every chunk.

## Validation

The portable tests cover isolated voxels, adjacent blocks, different materials,
greedy cuboids, negative and positive halo faces, strict budget overflow,
invalid coordinates, exact exposed-face area on a deterministic mixed-material
volume, and repeat-build determinism.

## Integration gate

Before using these meshes in the PS4 frame loop, the renderer still needs:
1. a GPU upload path and a defined vertex/index or quad expansion format;
2. a material/texture atlas and per-face UV assignment;
3. chunk dirty tracking and mesh rebuild scheduling after edits;
4. bounded upload queues and memory accounting for the PS4;
5. camera/frustum culling, depth handling, and visual validation on hardware.

Until those steps land and are exercised on a real console, the mesher is
foundational geometry code rather than a playable 3D rendering upgrade.

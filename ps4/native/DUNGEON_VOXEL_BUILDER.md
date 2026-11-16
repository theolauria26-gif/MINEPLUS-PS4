# Dungeon voxel room builder

`DungeonVoxelBuilder` stamps one room from the procedural dungeon graph into a
16x16x128 `VoxelChunk`. It clips by global chunk origin, emits a solid floor,
perimeter walls, and an optional ceiling. Room categories receive distinct
floor materials (dark stone, iron, runes, obsidian, or crystal).

The output can be passed to the existing CPU greedy mesher. This is still a
room-stamping primitive: corridor voxelization, door openings, multi-chunk
assembly, lighting, encounters, and live renderer integration are not included.

# Dungeon voxel builder

`DungeonVoxelBuilder` converts the procedural room graph into clipped
16x16x128 voxel chunks. Rooms receive category-specific floors, perimeter
walls, and optional ceilings. Connected orthogonally adjacent rooms receive
walkable floor corridors, side walls, and two-voxel-high door openings.

`build_layout_chunk` clears its output and assembles every room and corridor
that intersects the requested chunk. Global coordinates are clipped against
the chunk origin, so callers can request neighboring chunks independently.
Invalid room IDs, duplicate room IDs, non-adjacent graph edges, unsupported
corridor widths, and invalid materials/heights are rejected.

The assembled chunk can be passed to the CPU greedy mesher. This remains a
procedural geometry module, not yet connected to the live game renderer,
collision, lighting, encounter spawning, or persistent dungeon streaming.

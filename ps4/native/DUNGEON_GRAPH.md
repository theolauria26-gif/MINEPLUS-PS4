# Procedural dungeon graph

The dungeon graph module creates a deterministic, connected room-and-corridor
layout in tile coordinates. Every room after the entrance is attached to an
existing room, guaranteeing a spanning tree; optional links between adjacent
grid cells add alternate routes. The final two rooms are reserved for the
boss and exit. Treasure and puzzle rooms are assigned when the requested
size allows it.

This is a CPU layout contract, not yet a rendered or playable dungeon. A
voxel-room extruder, encounter director, traps, keys, boss logic, and connection
to the in-game world/save system remain separate integration tasks.

Run the portable test with:

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -Werror -I ps4/native \
  ps4/native/dungeon_graph.cpp ps4/native/tests/dungeon_graph_tests.cpp \
  -o /tmp/mineplus-dungeon-tests
/tmp/mineplus-dungeon-tests
```

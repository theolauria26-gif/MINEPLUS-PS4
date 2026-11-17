#include "dungeon_voxel_builder.hpp"
#include "voxel_mesher.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace mineplus;

int main() {
    DungeonRoom room;
    room.id = 0;
    room.x = 4;
    room.y = 5;
    room.width = 8;
    room.height = 7;
    room.type = DungeonRoomType::Boss;

    DungeonVoxelSettings settings;
    settings.floor_z = 24;
    settings.wall_height = 4;
    settings.wall_material = 8;
    settings.floor_material = 3;
    VoxelChunk chunk;
    assert(DungeonVoxelBuilder::build_room(room, settings, chunk));
    assert(chunk.get(4, 5, 24) == 15);
    assert(chunk.get(4, 5, 25) == 8);
    assert(chunk.get(5, 6, 25) == 0);
    assert(chunk.get(5, 6, 24) == 15);
    assert(chunk.get(11, 11, 29) == 0);

    std::vector<MeshQuad> quads;
    assert(VoxelMesher::build(chunk, quads));
    assert(!quads.empty());

    DungeonVoxelSettings clipped = settings;
    clipped.chunk_origin_x = 8;
    clipped.chunk_origin_y = 8;
    VoxelChunk partial;
    assert(DungeonVoxelBuilder::build_room(room, clipped, partial));
    assert(partial.get(0, 0, 24) == 15);
    assert(partial.get(3, 3, 24) == 15);
    assert(partial.get(0, 3, 25) == 8);

    DungeonLayout layout;
    DungeonRoom left;
    left.id = 10; left.grid_x = 0; left.grid_y = 0;
    left.x = 0; left.y = 2; left.width = 8; left.height = 8;
    left.type = DungeonRoomType::Start;
    DungeonRoom right;
    right.id = 11; right.grid_x = 1; right.grid_y = 0;
    right.x = 18; right.y = 2; right.width = 8; right.height = 8;
    right.type = DungeonRoomType::Treasure;
    layout.rooms.push_back(left);
    layout.rooms.push_back(right);
    layout.connections.push_back({10, 11, 2});

    VoxelChunk assembled;
    assert(DungeonVoxelBuilder::build_layout_chunk(layout, settings, assembled));
    assert(assembled.get(7, 5, 24) == 3);  // corridor floor
    assert(assembled.get(8, 5, 25) == 0);  // doorway is carved open
    assert(assembled.get(17, 5, 25) == 0);
    assert(assembled.get(10, 4, 25) == 8); // corridor side wall
    assert(assembled.get(10, 5, 25) == 0); // passage remains walkable
    assert(assembled.get(15, 5, 24) == 0); // outside this chunk

    DungeonVoxelSettings far_chunk = settings;
    far_chunk.chunk_origin_x = 16;
    VoxelChunk corridor_chunk;
    assert(DungeonVoxelBuilder::build_layout_chunk(layout, far_chunk, corridor_chunk));
    assert(corridor_chunk.get(0, 5, 24) == 3);
    assert(corridor_chunk.get(1, 5, 24) == 3);
    assert(corridor_chunk.get(4, 5, 24) == 14); // treasure room floor

    DungeonLayout invalid_layout = layout;
    invalid_layout.connections[0].to = 99;
    VoxelChunk invalid_output;
    assert(!DungeonVoxelBuilder::build_layout_chunk(invalid_layout, settings, invalid_output));
    invalid_layout = layout;
    invalid_layout.connections[0].corridor_width = 6;
    assert(!DungeonVoxelBuilder::build_layout_chunk(invalid_layout, settings, invalid_output));
    invalid_layout = layout;
    invalid_layout.rooms[1].grid_x = 3;
    assert(!DungeonVoxelBuilder::build_layout_chunk(invalid_layout, settings, invalid_output));

    DungeonVoxelSettings invalid = settings;
    invalid.floor_z = 127;
    VoxelChunk untouched;
    assert(!DungeonVoxelBuilder::build_room(room, invalid, untouched));
    assert(untouched.get(4, 5, 24) == 0);
    invalid = settings;
    invalid.wall_material = 0;
    assert(!DungeonVoxelBuilder::build_room(room, invalid, untouched));

    std::cout << "MINEPLUS dungeon voxel builder tests: OK\\n";
}

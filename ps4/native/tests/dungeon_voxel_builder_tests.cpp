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
    assert(chunk.get(4, 5, 24) == 15); // boss room uses obsidian floor
    assert(chunk.get(4, 5, 25) == 8);
    assert(chunk.get(5, 6, 25) == 0);  // room interior remains open
    assert(chunk.get(5, 6, 24) == 15);
    assert(chunk.get(11, 11, 29) == 0); // walls stop at configured height

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

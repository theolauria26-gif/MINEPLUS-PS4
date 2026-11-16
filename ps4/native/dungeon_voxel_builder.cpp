#include "dungeon_voxel_builder.hpp"

namespace mineplus {
namespace {
bool valid_material(std::uint8_t material) {
    return material > 0 && material < 32;
}

std::uint8_t floor_for_room(DungeonRoomType type, std::uint8_t fallback) {
    switch (type) {
        case DungeonRoomType::Start: return 8;   // dark stone
        case DungeonRoomType::Treasure: return 14; // iron-inlaid floor
        case DungeonRoomType::Puzzle: return 17; // rune floor
        case DungeonRoomType::Boss: return 15;   // obsidian
        case DungeonRoomType::Exit: return 6;    // crystal
        case DungeonRoomType::Combat: return fallback;
    }
    return fallback;
}
} // namespace

bool DungeonVoxelBuilder::build_room(const DungeonRoom& room,
                                     const DungeonVoxelSettings& settings,
                                     VoxelChunk& output) {
    if (room.width < 3 || room.height < 3 ||
        room.width > VoxelChunk::SIZE_X ||
        room.height > VoxelChunk::SIZE_Y ||
        settings.wall_height < 2 || settings.wall_height > 16 ||
        settings.floor_z < 1 ||
        settings.floor_z + settings.wall_height >= VoxelChunk::SIZE_Z ||
        !valid_material(settings.wall_material) ||
        !valid_material(settings.floor_material)) return false;

    const std::uint8_t floor = floor_for_room(room.type, settings.floor_material);
    const std::int32_t min_x = room.x;
    const std::int32_t min_y = room.y;
    const std::int32_t max_x = min_x + room.width - 1;
    const std::int32_t max_y = min_y + room.height - 1;

    for (std::int32_t gy = min_y; gy <= max_y; ++gy) {
        for (std::int32_t gx = min_x; gx <= max_x; ++gx) {
            const std::int32_t lx = gx - settings.chunk_origin_x;
            const std::int32_t ly = gy - settings.chunk_origin_y;
            if (lx < 0 || lx >= VoxelChunk::SIZE_X ||
                ly < 0 || ly >= VoxelChunk::SIZE_Y) continue;
            output.set(static_cast<int>(lx), static_cast<int>(ly),
                       settings.floor_z, floor);

            const bool boundary = gx == min_x || gx == max_x ||
                                  gy == min_y || gy == max_y;
            if (!boundary) continue;
            for (std::uint8_t dz = 1; dz <= settings.wall_height; ++dz)
                output.set(static_cast<int>(lx), static_cast<int>(ly),
                           settings.floor_z + dz, settings.wall_material);
            if (settings.build_ceiling)
                output.set(static_cast<int>(lx), static_cast<int>(ly),
                           settings.floor_z + settings.wall_height + 1,
                           settings.wall_material);
        }
    }
    return true;
}

} // namespace mineplus

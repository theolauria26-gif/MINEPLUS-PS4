#include "dungeon_voxel_builder.hpp"
#include <algorithm>
#include <cstddef>

namespace mineplus {
namespace {
bool valid_material(std::uint8_t material) {
    return material > 0 && material < 32;
}

bool valid_settings(const DungeonVoxelSettings& settings) {
    return settings.wall_height >= 2 && settings.wall_height <= 16 &&
           settings.floor_z >= 1 &&
           settings.floor_z + settings.wall_height + (settings.build_ceiling ? 1 : 0) <
               VoxelChunk::SIZE_Z &&
           valid_material(settings.wall_material) &&
           valid_material(settings.floor_material);
}

std::uint8_t floor_for_room(DungeonRoomType type, std::uint8_t fallback) {
    switch (type) {
        case DungeonRoomType::Start: return 8;
        case DungeonRoomType::Treasure: return 14;
        case DungeonRoomType::Puzzle: return 17;
        case DungeonRoomType::Boss: return 15;
        case DungeonRoomType::Exit: return 6;
        case DungeonRoomType::Combat: return fallback;
    }
    return fallback;
}

const DungeonRoom* find_room(const DungeonLayout& layout, std::uint32_t id) {
    for (const DungeonRoom& room : layout.rooms)
        if (room.id == id) return &room;
    return nullptr;
}

bool valid_room(const DungeonRoom& room) {
    return room.width >= 3 && room.height >= 3 &&
           room.width <= VoxelChunk::SIZE_X &&
           room.height <= VoxelChunk::SIZE_Y;
}

void set_global(VoxelChunk& output, const DungeonVoxelSettings& settings,
                std::int32_t gx, std::int32_t gy, std::int32_t z,
                std::uint8_t material) {
    const std::int32_t lx = gx - settings.chunk_origin_x;
    const std::int32_t ly = gy - settings.chunk_origin_y;
    if (lx >= 0 && lx < VoxelChunk::SIZE_X &&
        ly >= 0 && ly < VoxelChunk::SIZE_Y &&
        z >= 0 && z < VoxelChunk::SIZE_Z)
        output.set(lx, ly, z, material);
}

bool build_corridor(const DungeonRoom& a, const DungeonRoom& b,
                    const DungeonConnection& edge,
                    const DungeonVoxelSettings& settings,
                    VoxelChunk& output) {
    if (edge.corridor_width == 0 || edge.corridor_width > 5) return false;
    const bool horizontal = a.grid_y == b.grid_y &&
                            (a.grid_x + 1 == b.grid_x || b.grid_x + 1 == a.grid_x);
    const bool vertical = a.grid_x == b.grid_x &&
                          (a.grid_y + 1 == b.grid_y || b.grid_y + 1 == a.grid_y);
    if (!horizontal && !vertical) return false;

    if (horizontal) {
        const DungeonRoom& left = a.x < b.x ? a : b;
        const DungeonRoom& right = a.x < b.x ? b : a;
        const std::int32_t overlap_min = std::max(left.y, right.y);
        const std::int32_t overlap_max = std::min(
            left.y + left.height - 1, right.y + right.height - 1);
        if (overlap_max - overlap_min + 1 < edge.corridor_width ||
            left.x + left.width - 1 >= right.x) return false;
        const std::int32_t lane_min = overlap_min +
            (overlap_max - overlap_min + 1 - edge.corridor_width) / 2;
        const std::int32_t x_min = left.x + left.width - 1;
        const std::int32_t x_max = right.x;
        for (std::int32_t x = x_min; x <= x_max; ++x) {
            for (std::int32_t lane = 0; lane < edge.corridor_width; ++lane) {
                const std::int32_t y = lane_min + lane;
                set_global(output, settings, x, y, settings.floor_z,
                           settings.floor_material);
            }
            for (std::int32_t z = 1; z <= settings.wall_height; ++z) {
                set_global(output, settings, x, lane_min - 1,
                           settings.floor_z + z, settings.wall_material);
                set_global(output, settings, x, lane_min + edge.corridor_width,
                           settings.floor_z + z, settings.wall_material);
            }
        }
        for (std::int32_t z = 1; z <= 2; ++z)
            for (std::int32_t lane = 0; lane < edge.corridor_width; ++lane) {
                set_global(output, settings, x_min, lane_min + lane,
                           settings.floor_z + z, 0);
                set_global(output, settings, x_max, lane_min + lane,
                           settings.floor_z + z, 0);
            }
    } else {
        const DungeonRoom& top = a.y < b.y ? a : b;
        const DungeonRoom& bottom = a.y < b.y ? b : a;
        const std::int32_t overlap_min = std::max(top.x, bottom.x);
        const std::int32_t overlap_max = std::min(
            top.x + top.width - 1, bottom.x + bottom.width - 1);
        if (overlap_max - overlap_min + 1 < edge.corridor_width ||
            top.y + top.height - 1 >= bottom.y) return false;
        const std::int32_t lane_min = overlap_min +
            (overlap_max - overlap_min + 1 - edge.corridor_width) / 2;
        const std::int32_t y_min = top.y + top.height - 1;
        const std::int32_t y_max = bottom.y;
        for (std::int32_t y = y_min; y <= y_max; ++y) {
            for (std::int32_t lane = 0; lane < edge.corridor_width; ++lane)
                set_global(output, settings, lane_min + lane, y,
                           settings.floor_z, settings.floor_material);
            for (std::int32_t z = 1; z <= settings.wall_height; ++z) {
                set_global(output, settings, lane_min - 1, y,
                           settings.floor_z + z, settings.wall_material);
                set_global(output, settings, lane_min + edge.corridor_width, y,
                           settings.floor_z + z, settings.wall_material);
            }
        }
        for (std::int32_t z = 1; z <= 2; ++z)
            for (std::int32_t lane = 0; lane < edge.corridor_width; ++lane) {
                set_global(output, settings, lane_min + lane, y_min,
                           settings.floor_z + z, 0);
                set_global(output, settings, lane_min + lane, y_max,
                           settings.floor_z + z, 0);
            }
    }
    return true;
}
} // namespace

bool DungeonVoxelBuilder::build_room(const DungeonRoom& room,
                                     const DungeonVoxelSettings& settings,
                                     VoxelChunk& output) {
    if (!valid_room(room) || !valid_settings(settings)) return false;

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
            output.set(lx, ly, settings.floor_z, floor);
            const bool boundary = gx == min_x || gx == max_x ||
                                  gy == min_y || gy == max_y;
            if (!boundary) continue;
            for (std::uint8_t dz = 1; dz <= settings.wall_height; ++dz)
                output.set(lx, ly, settings.floor_z + dz, settings.wall_material);
            if (settings.build_ceiling)
                output.set(lx, ly, settings.floor_z + settings.wall_height + 1,
                           settings.wall_material);
        }
    }
    return true;
}

bool DungeonVoxelBuilder::build_layout_chunk(const DungeonLayout& layout,
                                              const DungeonVoxelSettings& settings,
                                              VoxelChunk& output) {
    if (!valid_settings(settings) || layout.rooms.empty() ||
        layout.rooms.size() > 256 || layout.connections.size() > 1024)
        return false;
    for (std::size_t i = 0; i < layout.rooms.size(); ++i) {
        if (!valid_room(layout.rooms[i])) return false;
        for (std::size_t j = i + 1; j < layout.rooms.size(); ++j)
            if (layout.rooms[i].id == layout.rooms[j].id) return false;
    }
    for (const DungeonConnection& edge : layout.connections) {
        const DungeonRoom* a = find_room(layout, edge.from);
        const DungeonRoom* b = find_room(layout, edge.to);
        if (!a || !b || a == b || edge.corridor_width == 0 ||
            edge.corridor_width > 5) return false;
        const bool adjacent = (a->grid_x == b->grid_x &&
            (a->grid_y + 1 == b->grid_y || b->grid_y + 1 == a->grid_y)) ||
            (a->grid_y == b->grid_y &&
            (a->grid_x + 1 == b->grid_x || b->grid_x + 1 == a->grid_x));
        if (!adjacent) return false;
    }

    output.cells.fill(0);
    for (const DungeonRoom& room : layout.rooms)
        if (!build_room(room, settings, output)) return false;
    for (const DungeonConnection& edge : layout.connections) {
        const DungeonRoom* a = find_room(layout, edge.from);
        const DungeonRoom* b = find_room(layout, edge.to);
        if (!build_corridor(*a, *b, edge, settings, output)) return false;
    }
    return true;
}

} // namespace mineplus

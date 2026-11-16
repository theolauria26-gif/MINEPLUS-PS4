#include "chunk_generator.hpp"
#include "chunk_cache.hpp"
#include "pcg_world.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

static bool same_sample(const mineplus::SurfaceSample& a,
                        const mineplus::SurfaceSample& b) {
    return a.height == b.height && a.surface == b.surface &&
           a.biome == b.biome && a.moisture == b.moisture &&
           a.temperature == b.temperature;
}

int main() {
    using namespace mineplus;

    assert(ChunkGenerator::floor_div(0, 16) == 0);
    assert(ChunkGenerator::floor_div(15, 16) == 0);
    assert(ChunkGenerator::floor_div(16, 16) == 1);
    assert(ChunkGenerator::floor_div(-1, 16) == -1);
    assert(ChunkGenerator::floor_div(-16, 16) == -1);
    assert(ChunkGenerator::floor_div(-17, 16) == -2);

    PcgWorld worldA(4839201ULL);
    PcgWorld worldB(4839201ULL);
    PcgWorld worldOther(4839202ULL);

    const std::int32_t coords[][2] = {
        {0, 0}, {1, 1}, {-1, -1}, {-16, 31},
        {-1024, 2048}, {4096, -8192}, {123456, -654321}
    };
    bool seedChangesTerrain = false;
    for (const auto& p : coords) {
        const auto a = worldA.sample_surface(p[0], p[1]);
        const auto b = worldB.sample_surface(p[0], p[1]);
        const auto c = worldOther.sample_surface(p[0], p[1]);
        assert(same_sample(a, b));
        if (!same_sample(a, c)) seedChangesTerrain = true;
    }
    assert(seedChangesTerrain);

    // 3D voxel queries are deterministic and bounded to the world height.
    for (const auto& p : coords) {
        const auto column = worldA.sample_surface(p[0], p[1]);
        const auto top1 = worldA.sample_voxel(p[0], p[1], column.height);
        const auto top2 = worldB.sample_voxel(p[0], p[1], column.height);
        assert(top1.solid && !top1.liquid && top1.material != VoxelMaterial::Air);
        assert(top1.material == top2.material);
        const auto soil = worldA.sample_voxel(p[0], p[1], column.height - 1);
        assert(soil.solid);
        const auto belowWorld = worldA.sample_voxel(p[0], p[1], -1);
        const auto aboveWorld = worldA.sample_voxel(p[0], p[1], PcgWorld::WORLD_HEIGHT);
        assert(belowWorld.material == VoxelMaterial::Air && !belowWorld.solid);
        assert(aboveWorld.material == VoxelMaterial::Air && !aboveWorld.solid);
    }
    const auto plainsColumn = worldA.sample_surface(0, 0);
    const auto deepA = worldA.sample_voxel(0, 0, plainsColumn.height - 8);
    const auto deepB = worldB.sample_voxel(0, 0, plainsColumn.height - 8);
    assert(deepA.material == deepB.material && deepA.solid == deepB.solid);

    StructureAnchor a{}, b{};
    const bool hasA = worldA.structure_anchor_for_cell(-7, 12, a);
    const bool hasB = worldB.structure_anchor_for_cell(-7, 12, b);
    assert(hasA == hasB);
    if (hasA) {
        assert(a.x == b.x && a.y == b.y &&
               a.kind == b.kind && a.radius == b.radius);
    }

    // Landmark anchors also create a deterministic primordial complex:
    // five chambers connected by a cross corridor, with a solid roof and floor.
    bool checkedDungeon = false;
    for (int cy = -30; cy <= 30 && !checkedDungeon; ++cy) {
        for (int cx = -30; cx <= 30 && !checkedDungeon; ++cx) {
            StructureAnchor dungeon{};
            if (!worldA.structure_anchor_for_cell(cx, cy, dungeon)) continue;
            const auto surface = worldA.sample_surface(dungeon.x, dungeon.y);
            if (surface.height < 15) continue;
            const auto room = worldA.sample_voxel(dungeon.x, dungeon.y,
                                                   surface.height - 11);
            const auto floor = worldA.sample_voxel(dungeon.x, dungeon.y,
                                                   surface.height - 14);
            const auto roof = worldA.sample_voxel(dungeon.x, dungeon.y,
                                                   surface.height - 8);
            const auto repeatRoom = worldB.sample_voxel(dungeon.x, dungeon.y,
                                                         surface.height - 11);
            assert(room.material == VoxelMaterial::Air && !room.solid);
            assert(floor.material == VoxelMaterial::DarkStone && floor.solid);
            assert(roof.material == VoxelMaterial::DarkStone && roof.solid);
            assert(room.material == repeatRoom.material);

            const int offset = std::max(3, int(dungeon.radius) * 2 / 3);
            const int wingRadius = std::max(2, int(dungeon.radius) / 5);
            const int wingX = dungeon.x + offset;
            const int wingY = dungeon.y;
            const auto eastSurface = worldA.sample_surface(wingX, wingY);
            const auto northSurface = worldA.sample_surface(dungeon.x, dungeon.y - offset);
            const auto southSurface = worldA.sample_surface(dungeon.x, dungeon.y + offset);
            const auto westSurface = worldA.sample_surface(dungeon.x - offset, dungeon.y);
            const auto eastWing = worldA.sample_voxel(
                wingX, wingY, eastSurface.height - 11);
            const auto northWing = worldA.sample_voxel(
                dungeon.x, dungeon.y - offset, northSurface.height - 11);
            const auto southWing = worldA.sample_voxel(
                dungeon.x, dungeon.y + offset, southSurface.height - 11);
            const auto westWing = worldA.sample_voxel(
                dungeon.x - offset, dungeon.y, westSurface.height - 11);
            const auto isWalkable = [](const VoxelSample& v) {
                return v.material == VoxelMaterial::Air ||
                       v.material == VoxelMaterial::Crystal;
            };
            assert(isWalkable(eastWing));
            assert(isWalkable(northWing));
            assert(isWalkable(southWing));
            assert(isWalkable(westWing));
            const auto eastEdge = worldA.sample_voxel(
                wingX + wingRadius, wingY,
                worldA.sample_surface(wingX + wingRadius, wingY).height - 11);
            assert(isWalkable(eastEdge));
            checkedDungeon = true;
        }
    }
    assert(checkedDungeon);

    ChunkGenerator genA(4839201ULL), genB(4839201ULL);
    ChunkData left{}, repeat{}, right{};
    genA.generate(-1, -1, left);
    genB.generate(-1, -1, repeat);
    genA.generate(0, -1, right);

    for (std::size_t i = 0; i < left.cells.size(); ++i)
        assert(same_sample(left.cells[i], repeat.cells[i]));

    // Adjacent chunks must agree with direct global-coordinate sampling.
    assert(same_sample(left.cells[15], worldA.sample_surface(-1, -16)));
    assert(same_sample(right.cells[0], worldA.sample_surface(0, -16)));
    assert(same_sample(left.cells[0], worldA.sample_surface(-16, -16)));

    static_assert(sizeof(ChunkCache) <= 512u * 1024u,
                  "PS4 chunk cache must stay within the 512 KiB memory budget");

    ChunkCache cache(4839201ULL);
    cache.update_center(0, 0, 4);
    assert(cache.loaded_count() == ChunkCache::CAPACITY);
    SurfaceSample cached{};
    assert(cache.get(-1, -1, cached));
    assert(same_sample(cached, worldA.sample_surface(-1, -1)));
    assert(cache.set_surface(-1, -1, 77));
    assert(cache.get(-1, -1, cached) && cached.surface == 77);
    assert(cache.delta_count() == 1);
    cache.update_center(4096, 4096, 4);
    assert(cache.get(-1, -1, cached) && cached.surface == 77);
    assert(cache.delta_count() == 1);

    // Edit journal export/import supports compact durable-save integration.
    ChunkCache persisted(4839201ULL);
    persisted.set_surface(-33, 48, 91);
    persisted.set_surface(700, -900, 92);
    ChunkCache restored(4839201ULL);
    ChunkCache::Edit saved[ChunkCache::DELTA_CAPACITY]{};
    const auto savedCount = persisted.export_edits(saved, ChunkCache::DELTA_CAPACITY);
    assert(savedCount == 2);
    assert(restored.import_edits(saved, savedCount));
    assert(restored.get(-33, 48, cached) && cached.surface == 91);
    assert(restored.get(700, -900, cached) && cached.surface == 92);
    const ChunkCache::Edit duplicate[] = {{1, 2, 3}, {1, 2, 4}};
    assert(!restored.import_edits(duplicate, 2));
    assert(restored.delta_count() == 2);

    // The bounded edit journal must reject a new edit when full, but still
    // allow updating an existing coordinate without losing its old record.
    ChunkCache fullJournal(4839201ULL);
    for (std::int32_t i = 0; i < static_cast<std::int32_t>(ChunkCache::DELTA_CAPACITY); ++i) {
        assert(fullJournal.set_surface(i, 7000, static_cast<std::uint8_t>(i & 0xff)));
    }
    assert(fullJournal.delta_count() == ChunkCache::DELTA_CAPACITY);
    assert(!fullJournal.set_surface(9000, 7000, 12));
    assert(fullJournal.set_surface(0, 7000, 99));
    SurfaceSample edited{};
    assert(fullJournal.get(0, 7000, edited) && edited.surface == 99);
    assert(fullJournal.delta_count() == ChunkCache::DELTA_CAPACITY);

    std::cout << "MINEPLUS PCG/chunk tests: OK\n";
    return 0;
}

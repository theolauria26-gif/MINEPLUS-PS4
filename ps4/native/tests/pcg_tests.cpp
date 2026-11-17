#include "chunk_generator.hpp"
#include "chunk_cache.hpp"
#include "pcg_world.hpp"
#include <cassert>
#include <cmath>
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

    // The overworld must contain deterministic river channels and sandy
    // transitions without erasing the identity of the surrounding biome.
    bool foundRiver = false;
    bool foundRiverBank = false;
    for (int y = -2048; y <= 2048; y += 8) {
        for (int x = -2048; x <= 2048; x += 8) {
            const auto sample = worldA.sample_surface(x, y);
            if (sample.surface == 13 && sample.biome !=
                    static_cast<std::uint8_t>(Biome::Ocean)) {
                foundRiver = true;
                const auto water = worldA.sample_voxel(x, y, sample.height);
                assert(water.material == VoxelMaterial::Water);
                assert(water.liquid && !water.solid);
                assert(same_sample(sample, worldB.sample_surface(x, y)));
            }
            if (sample.surface == 9 &&
                sample.biome != static_cast<std::uint8_t>(Biome::Desert) &&
                sample.biome != static_cast<std::uint8_t>(Biome::Ocean)) {
                foundRiverBank = true;
                const auto bank = worldA.sample_voxel(x, y, sample.height);
                assert(bank.material == VoxelMaterial::Sand && bank.solid);
            }
        }
    }
    assert(foundRiver);
    assert(foundRiverBank);

    // Forests grow in broad deterministic groves; wet plains may contain
    // occasional trees, while deserts and oceans remain free of tree spawns.
    std::uint32_t forestTrees = 0;
    std::uint32_t plainsTrees = 0;
    bool checkedTreeRepeat = false;
    for (int y = -4096; y <= 4096; y += 16) {
        for (int x = -4096; x <= 4096; x += 16) {
            const auto column = worldA.sample_surface(x, y);
            const bool tree = worldA.surface_tree_at(x, y, column);
            assert(tree == worldB.surface_tree_at(x, y, column));
            const auto biome = static_cast<Biome>(column.biome);
            if (tree && biome == Biome::Forest) ++forestTrees;
            if (tree && biome == Biome::Plains) ++plainsTrees;
            if (tree) {
                assert(column.surface == 1 || column.surface == 5);
                checkedTreeRepeat = true;
            }
            if (biome == Biome::Desert || biome == Biome::Ocean)
                assert(!tree);
        }
    }
    assert(checkedTreeRepeat);
    assert(forestTrees > plainsTrees);

    // Inland lakes form broad, deterministic pools rather than thin river
    // channels. A five-point cross verifies a stable liquid basin core.
    bool foundLakePool = false;
    for (int y = -4096; y <= 4096 && !foundLakePool; y += 8) {
        for (int x = -4096; x <= 4096 && !foundLakePool; x += 8) {
            const int offsets[][2] = {{0,0},{-4,0},{4,0},{0,-4},{0,4}};
            bool pool = true;
            for (const auto& offset : offsets) {
                const int wx = x + offset[0], wy = y + offset[1];
                const auto sample = worldA.sample_surface(wx, wy);
                if (sample.surface != 13 ||
                    sample.biome == static_cast<std::uint8_t>(Biome::Ocean)) {
                    pool = false;
                    break;
                }
                const auto voxel = worldA.sample_voxel(wx, wy, sample.height);
                assert(voxel.material == VoxelMaterial::Water);
                assert(voxel.liquid && !voxel.solid);
                assert(same_sample(sample, worldB.sample_surface(wx, wy)));
            }
            if (pool) foundLakePool = true;
        }
    }
    assert(foundLakePool);

    // Mountain chains and valleys add a broader elevation silhouette while
    // remaining deterministic at global coordinates and across world instances.
    std::int16_t lowestSurface = PcgWorld::WORLD_HEIGHT;
    std::int16_t highestSurface = 0;
    std::uint32_t highlandColumns = 0;
    for (int y = -4096; y <= 4096; y += 32) {
        for (int x = -4096; x <= 4096; x += 32) {
            const auto a = worldA.sample_surface(x, y);
            const auto b = worldB.sample_surface(x, y);
            assert(same_sample(a, b));
            lowestSurface = std::min(lowestSurface, a.height);
            highestSurface = std::max(highestSurface, a.height);
            if (a.biome == static_cast<std::uint8_t>(Biome::Highlands)) {
                ++highlandColumns;
                assert(a.height >= 40);
            }
        }
    }
    assert(highlandColumns > 0);
    assert(highestSurface - lowestSurface >= 20);

    // The central continent transitions into warm sandy beaches before
    // the ocean shelf; coastline samples remain deterministic across seeds.
    bool foundCoastalBeach = false;
    for (int y = -5200; y <= 5200; y += 24) {
        for (int x = -5200; x <= 5200; x += 24) {
            const float distance = std::sqrt(float(x) * float(x) +
                                             float(y) * float(y));
            if (distance < 4550.0f || distance > 5000.0f) continue;
            const auto a = worldA.sample_surface(x, y);
            const auto b = worldB.sample_surface(x, y);
            assert(same_sample(a, b));
            if (a.surface == 9 &&
                a.biome != static_cast<std::uint8_t>(Biome::Desert) &&
                a.biome != static_cast<std::uint8_t>(Biome::Ocean)) {
                foundCoastalBeach = true;
                const auto sand = worldA.sample_voxel(x, y, a.height);
                assert(sand.material == VoxelMaterial::Sand && sand.solid);
            }
        }
    }
    assert(foundCoastalBeach);

    // 3D voxel queries are deterministic and bounded to the world height.
    for (const auto& p : coords) {
        const auto column = worldA.sample_surface(p[0], p[1]);
        const auto top1 = worldA.sample_voxel(p[0], p[1], column.height);
        const auto top2 = worldB.sample_voxel(p[0], p[1], column.height);
        if (top1.liquid) {
            assert(top1.material == VoxelMaterial::Water && !top1.solid);
        } else {
            assert(top1.solid && top1.material != VoxelMaterial::Air);
        }
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

    // Surface camps are a fifth deterministic landmark family. Verify that
    // the expanded anchor catalog is stable and that camps occur in the search area.
    bool foundSurfaceCamp = false;
    bool foundFrontierWatchPost = false;
    for (int cy = -24; cy <= 24; ++cy) {
        for (int cx = -24; cx <= 24; ++cx) {
            StructureAnchor campA{}, campB{};
            const bool hasCampA = worldA.structure_anchor_for_cell(cx, cy, campA);
            const bool hasCampB = worldB.structure_anchor_for_cell(cx, cy, campB);
            assert(hasCampA == hasCampB);
            if (!hasCampA) continue;
            assert(campA.kind >= 1 && campA.kind <= 6);
            assert(campA.x == campB.x && campA.y == campB.y &&
                   campA.kind == campB.kind && campA.radius == campB.radius);
            if (campA.kind == 5) foundSurfaceCamp = true;
            if (campA.kind == 6) foundFrontierWatchPost = true;
        }
    }
    assert(foundSurfaceCamp);
    assert(foundFrontierWatchPost);

    // Road links select the same nearest landmark pair for identical seeds.
    int roadLinks = 0;
    for (int cy = -24; cy <= 24; ++cy) {
        for (int cx = -24; cx <= 24; ++cx) {
            StructureAnchor fromA{}, toA{}, fromB{}, toB{};
            const bool linkA = worldA.surface_road_link_for_cell(cx, cy, fromA, toA);
            const bool linkB = worldB.surface_road_link_for_cell(cx, cy, fromB, toB);
            assert(linkA == linkB);
            if (!linkA) continue;
            ++roadLinks;
            assert(fromA.x == fromB.x && fromA.y == fromB.y);
            assert(toA.x == toB.x && toA.y == toB.y);
            assert(fromA.x != toA.x || fromA.y != toA.y);
        }
    }
    assert(roadLinks > 0);

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

            // Primordial chambers contain deterministic floor-level crystal
            // deposits. Scan the complete central chamber and verify the same
            // coordinates/materials in a second world with the same seed.
            bool foundFloorVein = false;
            const int scanRadius = int(dungeon.radius);
            for (int vy = -scanRadius; vy <= scanRadius; ++vy) {
                for (int vx = -scanRadius; vx <= scanRadius; ++vx) {
                    const int wx = dungeon.x + vx;
                    const int wy = dungeon.y + vy;
                    const auto localSurface = worldA.sample_surface(wx, wy);
                    const auto veinA = worldA.sample_voxel(
                        wx, wy, localSurface.height - 13);
                    const auto veinB = worldB.sample_voxel(
                        wx, wy, localSurface.height - 13);
                    assert(veinA.material == veinB.material);
                    if (veinA.material == VoxelMaterial::Crystal) {
                        foundFloorVein = true;
                        assert(veinA.solid && !veinA.liquid);
                    }
                }
            }
            assert(foundFloorVein);

            const int profile = int(dungeon.kind % 4u);
            const int mainDivisor = profile == 1 ? 3 : (profile == 2 ? 5 : 4);
            const int wingDivisor = profile == 2 ? 6 : (profile == 3 ? 4 : 5);
            const int offsetNumerator = profile == 1 ? 3 : (profile == 2 ? 7 : 2);
            const int offsetDenominator = profile == 1 ? 5 : (profile == 2 ? 10 : 3);
            const int mainRadius = std::max(2, int(dungeon.radius) / mainDivisor);
            const int offset = std::max(3, int(dungeon.radius) *
                                           offsetNumerator / offsetDenominator);
            const int wingRadius = std::max(2, int(dungeon.radius) / wingDivisor);
            assert(mainRadius >= 2);

            // Four rune pylons and their ember caps are deterministic, solid
            // landmarks at the central chamber's diagonal corners.
            const int pylonOffset = std::max(1, mainRadius / 2);
            for (int signX : {-1, 1}) {
                for (int signY : {-1, 1}) {
                    const int px = dungeon.x + signX * pylonOffset;
                    const int py = dungeon.y + signY * pylonOffset;
                    const auto pylonSurface = worldA.sample_surface(px, py);
                    const auto rune = worldA.sample_voxel(
                        px, py, pylonSurface.height - 11);
                    const auto ember = worldA.sample_voxel(
                        px, py, pylonSurface.height - 10);
                    const auto repeatedRune = worldB.sample_voxel(
                        px, py, pylonSurface.height - 11);
                    assert(rune.material == VoxelMaterial::Rune && rune.solid);
                    assert(ember.material == VoxelMaterial::Ember && ember.solid);
                    assert(repeatedRune.material == rune.material);
                }
            }

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
            const auto southEmber = worldA.sample_voxel(
                dungeon.x, dungeon.y + offset, southSurface.height - 10);
            const auto westCap = worldA.sample_voxel(
                dungeon.x - offset, dungeon.y, westSurface.height - 10);
            const auto eastRelic = worldA.sample_voxel(
                wingX, wingY, eastSurface.height - 11);
            const auto isWalkable = [](const VoxelSample& v) {
                return v.material == VoxelMaterial::Air ||
                       v.material == VoxelMaterial::Crystal;
            };
            assert(northWing.material == VoxelMaterial::Crystal && northWing.solid);
            assert(southWing.material == VoxelMaterial::Rune && southWing.solid);
            assert(southEmber.material == VoxelMaterial::Ember && southEmber.solid);
            assert(westWing.material == VoxelMaterial::Obsidian && westWing.solid);
            assert(westCap.material == VoxelMaterial::DarkStone && westCap.solid);
            assert(eastRelic.material == VoxelMaterial::Bone && eastRelic.solid);
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

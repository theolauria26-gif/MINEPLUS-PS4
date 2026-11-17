#include "world_runtime.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

static_assert(sizeof(mineplus::ChunkCache) <= 512u * 1024u,
              "PS4 streamed chunk cache exceeded its static memory budget");

int main() {
    // Signed world coordinates must map to mathematically correct chunks,
    // including the int32 limits used by the streamed-world API.
    using mineplus::ChunkGenerator;
    assert(ChunkGenerator::floor_div(std::numeric_limits<std::int32_t>::min(), 16) ==
           -134217728);
    assert(ChunkGenerator::floor_div(std::numeric_limits<std::int32_t>::max(), 16) ==
           134217727);
    assert(ChunkGenerator::floor_div(-1, 16) == -1);
    assert(ChunkGenerator::floor_div(-16, 16) == -1);
    assert(ChunkGenerator::floor_div(-17, 16) == -2);
    mineplus::WorldRuntime world(4839201ULL);
    assert(world.seed() == 4839201ULL);
    world.update_center(0, 0, 1);
    assert(world.loaded_chunks() == 9);

    mineplus::PcgWorld anchorGenerator(4839201ULL);
    mineplus::StructureAnchor expectedAnchor{};
    bool foundAnchor = false;
    for (int cy = -20; cy <= 20 && !foundAnchor; ++cy) {
        for (int cx = -20; cx <= 20 && !foundAnchor; ++cx) {
            foundAnchor = anchorGenerator.structure_anchor_for_cell(cx, cy, expectedAnchor);
        }
    }
    assert(foundAnchor);
    mineplus::StructureAnchor cachedAnchor{};
    assert(world.get_structure(expectedAnchor.x, expectedAnchor.y, cachedAnchor));
    assert(cachedAnchor.x == expectedAnchor.x && cachedAnchor.y == expectedAnchor.y);
    assert(cachedAnchor.kind == expectedAnchor.kind);

    mineplus::SurfaceSample originalSample{};
    assert(world.get_sample(-1, -1, originalSample));
    std::uint8_t before = 0;
    assert(world.get_tile(-1, -1, before));
    assert(before == originalSample.surface);
    assert(world.set_tile(-1, -1, 255));
    std::uint8_t edited = 0;
    assert(world.get_tile(-1, -1, edited) && edited == 255);
    assert(world.saved_edits() == 1);
    mineplus::VoxelSample voxel{};
    assert(world.get_voxel(-1, -1, originalSample.height, voxel));
    assert(voxel.material == mineplus::VoxelMaterial::Stone);
    const int editedDepth = originalSample.height - 2;
    assert(world.set_voxel(-1, -1, editedDepth, mineplus::VoxelMaterial::Crystal));
    assert(world.saved_edits() == 2); // Surface + 3D voxel journals are both counted.
    assert(world.set_voxel(-1, -1, editedDepth, mineplus::VoxelMaterial::Obsidian));
    assert(world.saved_edits() == 2); // Replacing one coordinate does not add a duplicate.
    assert(world.set_voxel(-1, -1, editedDepth, mineplus::VoxelMaterial::Crystal));
    assert(world.get_voxel(-1, -1, editedDepth, voxel));
    assert(voxel.material == mineplus::VoxelMaterial::Crystal && voxel.solid);

    // Invalid or duplicate imports must not erase the already valid voxel journal.
    const mineplus::ChunkCache::VoxelEdit invalidVoxel[] = {
        {-1, -1, mineplus::PcgWorld::WORLD_HEIGHT, mineplus::VoxelMaterial::Stone}
    };
    assert(!world.import_voxel_edits(invalidVoxel, 1));
    const mineplus::ChunkCache::VoxelEdit duplicateVoxel[] = {
        {-1, -1, editedDepth, mineplus::VoxelMaterial::Stone},
        {-1, -1, editedDepth, mineplus::VoxelMaterial::Air}
    };
    assert(!world.import_voxel_edits(duplicateVoxel, 2));
    assert(world.saved_edits() == 2);
    assert(world.get_voxel(-1, -1, editedDepth, voxel));
    assert(voxel.material == mineplus::VoxelMaterial::Crystal);

    // Moving far enough evicts the original chunk; edits and base metadata persist.
    world.update_center(4096, -4096, 4);
    assert(world.loaded_chunks() == 81);
    assert(world.get_tile(-1, -1, edited) && edited == 255);
    mineplus::SurfaceSample reloadedSample{};
    assert(world.get_sample(-1, -1, reloadedSample));
    assert(reloadedSample.surface == 255);
    assert(reloadedSample.biome == originalSample.biome);
    assert(reloadedSample.height == originalSample.height);
    assert(world.get_voxel(-1, -1, reloadedSample.height, voxel));
    assert(voxel.material == mineplus::VoxelMaterial::Stone);
    assert(world.get_voxel(-1, -1, editedDepth, voxel));
    assert(voxel.material == mineplus::VoxelMaterial::Crystal);


    mineplus::ChunkCache::Edit saved[4]{};
    assert(world.export_edits(saved, 4) == 1);
    mineplus::WorldRuntime restored(4839201ULL);
    assert(restored.import_edits(saved, 1));
    assert(restored.get_tile(-1, -1, edited) && edited == 255);

    mineplus::DungeonChestState chest{123,false,{{mineplus::DungeonLootItem::CrystalShard,5},{mineplus::DungeonLootItem::RuneFragment,2}}};
    assert(world.set_dungeon_chest(chest));
    mineplus::DungeonChestState fetchedChest{};
    assert(world.get_dungeon_chest(123,fetchedChest));
    assert(!fetchedChest.opened && fetchedChest.contents.size()==2);
    auto openedChest=fetchedChest; openedChest.opened=true;
    assert(world.set_dungeon_chest(openedChest));
    assert(!world.set_dungeon_chest(mineplus::DungeonChestState{124,false,{{mineplus::DungeonLootItem::BossSigil,0}}}));

    std::vector<std::uint8_t> snapshot;
    assert(world.encode_save(snapshot));
    mineplus::WorldRuntime diskLoaded(1);
    assert(diskLoaded.decode_save(snapshot.data(), snapshot.size()));
    assert(diskLoaded.seed() == world.seed());
    assert(diskLoaded.loaded_chunks() == 0);
    assert(diskLoaded.get_tile(-1, -1, edited) && edited == 255);
    assert(diskLoaded.get_voxel(-1, -1, editedDepth, voxel));
    assert(voxel.material == mineplus::VoxelMaterial::Crystal);
    mineplus::mineplus::DungeonChestState restoredChest{};
    assert(diskLoaded.get_dungeon_chest(123,restoredChest));
    assert(restoredChest.opened && restoredChest.contents.size()==2);
    assert(restoredChest.contents[0].item==mineplus::mineplus::DungeonLootItem::CrystalShard &&
           restoredChest.contents[0].count==5);
    auto corrupt = snapshot;
    corrupt.back() ^= 0x80;
    const auto previousSeed = diskLoaded.seed();
    assert(!diskLoaded.decode_save(corrupt.data(), corrupt.size()));
    assert(diskLoaded.seed() == previousSeed);
    assert(diskLoaded.get_tile(-1, -1, edited) && edited == 255);

    const mineplus::ChunkCache::Edit duplicate[] = {{7, 8, 1}, {7, 8, 2}};
    assert(!restored.import_edits(duplicate, 2));
    std::cout << "MINEPLUS world runtime tests: OK\n";
}

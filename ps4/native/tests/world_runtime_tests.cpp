#include "world_runtime.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

static_assert(sizeof(mineplus::ChunkCache) <= 512u * 1024u,
              "PS4 streamed chunk cache exceeded its static memory budget");

int main() {
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


    mineplus::ChunkCache::Edit saved[4]{};
    assert(world.export_edits(saved, 4) == 1);
    mineplus::WorldRuntime restored(4839201ULL);
    assert(restored.import_edits(saved, 1));
    assert(restored.get_tile(-1, -1, edited) && edited == 255);

    std::vector<std::uint8_t> snapshot;
    assert(world.encode_save(snapshot));
    mineplus::WorldRuntime diskLoaded(1);
    assert(diskLoaded.decode_save(snapshot.data(), snapshot.size()));
    assert(diskLoaded.seed() == world.seed());
    assert(diskLoaded.loaded_chunks() == 0);
    assert(diskLoaded.get_tile(-1, -1, edited) && edited == 255);
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

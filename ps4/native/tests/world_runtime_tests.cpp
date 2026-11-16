#include "world_runtime.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    mineplus::WorldRuntime world(4839201ULL);
    assert(world.seed() == 4839201ULL);

    world.update_center(0, 0, 1);
    assert(world.loaded_chunks() == 9);

    std::uint8_t before = 0;
    assert(world.get_tile(-1, -1, before));
    assert(world.set_tile(-1, -1, 255));
    std::uint8_t edited = 0;
    assert(world.get_tile(-1, -1, edited) && edited == 255);
    assert(world.saved_edits() == 1);

    // Moving far enough evicts the original chunk; the journal must restore it.
    world.update_center(4096, -4096, 4);
    assert(world.loaded_chunks() == 81);
    assert(world.get_tile(-1, -1, edited) && edited == 255);

    mineplus::ChunkCache::Edit saved[4]{};
    assert(world.export_edits(saved, 4) == 1);
    mineplus::WorldRuntime restored(4839201ULL);
    assert(restored.import_edits(saved, 1));
    assert(restored.get_tile(-1, -1, edited) && edited == 255);
    const mineplus::ChunkCache::Edit duplicate[] = {{7, 8, 1}, {7, 8, 2}};
    assert(!restored.import_edits(duplicate, 2));
    std::cout << "MINEPLUS world runtime tests: OK\n";
}

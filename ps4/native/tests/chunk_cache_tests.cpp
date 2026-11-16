#include "chunk_cache.hpp"
#include <cassert>
#include <iostream>

int main() {
    mineplus::ChunkCache cache(4839201ULL);
    assert(!cache.set_surface(0, 0, 255));
    assert(cache.set_surface(-1, -1, 9));
    mineplus::SurfaceSample sample{};
    assert(cache.get(-1, -1, sample));
    assert(sample.surface == 9);

    const mineplus::ChunkCache::Edit duplicate[] = {{4, 5, 255}, {4, 5, 3}};
    assert(!cache.import_edits(duplicate, 2));
    assert(cache.delta_count() == 1);

    const mineplus::ChunkCache::Edit valid[] = {{32, -17, 255}, {-48, 64, 0}};
    assert(cache.import_edits(valid, 2));
    assert(cache.delta_count() == 2);
    assert(cache.get(32, -17, sample));
    assert(sample.surface == 255);
    std::cout << "MINEPLUS chunk cache validation tests: OK\\n";
}

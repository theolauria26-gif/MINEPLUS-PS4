#include "world_grid.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    std::uint8_t cells[12]{};
    mineplus::FixedWorldGrid fixed(cells, 4, 3);
    assert(fixed.width() == 4 && fixed.height() == 3);
    assert(fixed.contains(0, 0) && fixed.contains(3, 2));
    assert(!fixed.contains(-1, 0) && !fixed.contains(0, -1));
    assert(!fixed.contains(4, 2) && !fixed.contains(3, 3));
    assert(fixed.set(3, 2, 255));
    std::uint8_t tile = 0;
    assert(fixed.get(3, 2, tile) && tile == 255);
    assert(!fixed.get(-1, 0, tile));
    assert(!fixed.set(4, 2, 7));
    assert(cells[11] == 255);

    mineplus::FixedWorldGrid invalid(nullptr, 4, 3);
    assert(!invalid.get(0, 0, tile));
    assert(!invalid.set(0, 0, 1));

    mineplus::WorldRuntime runtime(99);
    runtime.update_center(0, 0, 1);
    mineplus::StreamedWorldGrid streamed(runtime);
    assert(streamed.contains(-1000000, 1000000));
    assert(streamed.set(-17, 33, 255));
    assert(streamed.get(-17, 33, tile) && tile == 255);
    mineplus::SurfaceSample sample{};
    assert(streamed.get_sample(-17, 33, sample));
    assert(sample.surface == 255);
    mineplus::VoxelSample voxel{};
    assert(streamed.get_voxel(-17, 33, sample.height, voxel));
    assert(voxel.solid && voxel.material != mineplus::VoxelMaterial::Air);
    assert(streamed.get_voxel(-17, 33, sample.height - 8, voxel));
    assert(streamed.get_voxel(-17, 33, -1, voxel));
    assert(!voxel.solid && voxel.material == mineplus::VoxelMaterial::Air);
    runtime.update_center(4096, -4096, 4);
    assert(streamed.get(-17, 33, tile) && tile == 255);
    assert(streamed.get_sample(-17, 33, sample) && sample.surface == 255);
    assert(runtime.saved_edits() == 1);

    mineplus::WorldGridView* active = &streamed;
    assert(active->get(-17, 33, tile) && tile == 255);
    std::cout << "MINEPLUS world grid tests: OK\n";
}

#include "world_grid.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    std::uint8_t cells[12]{};
    mineplus::FixedWorldGrid grid(cells, 4, 3);
    assert(grid.width() == 4 && grid.height() == 3);
    assert(grid.contains(0, 0) && grid.contains(3, 2));
    assert(!grid.contains(-1, 0) && !grid.contains(0, -1));
    assert(!grid.contains(4, 2) && !grid.contains(3, 3));

    assert(grid.set(3, 2, 255));
    std::uint8_t tile = 0;
    assert(grid.get(3, 2, tile) && tile == 255);
    assert(!grid.get(-1, 0, tile));
    assert(!grid.set(4, 2, 7));
    assert(cells[11] == 255);

    mineplus::FixedWorldGrid invalid(nullptr, 4, 3);
    assert(!invalid.get(0, 0, tile));
    assert(!invalid.set(0, 0, 1));
    std::cout << "MINEPLUS world grid tests: OK\n";
}

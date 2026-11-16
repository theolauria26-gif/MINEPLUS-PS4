#include "chunk_generator.hpp"
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

    StructureAnchor a{}, b{};
    const bool hasA = worldA.structure_anchor_for_cell(-7, 12, a);
    const bool hasB = worldB.structure_anchor_for_cell(-7, 12, b);
    assert(hasA == hasB);
    if (hasA) {
        assert(a.x == b.x && a.y == b.y &&
               a.kind == b.kind && a.radius == b.radius);
    }

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

    std::cout << "MINEPLUS PCG/chunk tests: OK\n";
    return 0;
}

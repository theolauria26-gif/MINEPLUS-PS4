#pragma once
#include "pcg_world.hpp"
#include <array>
#include <cstdint>

namespace mineplus {

// Um chunk guarda somente a superfície visível e os dados necessários para
// construir a representação do cliente. Sem SDL/GPU: pode ser produzido por worker.
struct ChunkData {
    std::int32_t chunk_x = 0;
    std::int32_t chunk_y = 0;
    std::uint64_t seed = 0;
    std::array<SurfaceSample, PcgWorld::CHUNK_SIZE * PcgWorld::CHUNK_SIZE> cells{};
    std::array<StructureAnchor, PcgWorld::CHUNK_SIZE * PcgWorld::CHUNK_SIZE> structures{};
    std::array<std::uint8_t, PcgWorld::CHUNK_SIZE * PcgWorld::CHUNK_SIZE> has_structure{};
};

class ChunkGenerator {
public:
    explicit ChunkGenerator(std::uint64_t seed) : world_(seed) {}
    // Geração pura: mesma seed e coordenadas sempre retornam os mesmos bytes.
    void generate(std::int32_t chunk_x, std::int32_t chunk_y, ChunkData& out) const;
    VoxelSample sample_voxel(std::int32_t x, std::int32_t y, std::int32_t z) const;
    static std::int32_t floor_div(std::int32_t value, std::int32_t divisor);
private:
    PcgWorld world_;
};

} // namespace mineplus

#pragma once
#include <cstdint>

namespace mineplus {

// Dados puros de uma coluna; a geração não cria objetos SDL e pode rodar em worker.
enum class Biome : std::uint8_t {
    Ocean, Plains, Forest, Desert, Tundra, Highlands, Volcanic, CrystalWastes
};

struct SurfaceSample {
    std::int16_t height;
    std::uint8_t surface;
    std::uint8_t biome;
    std::uint8_t moisture;
    std::uint8_t temperature;
};

struct StructureAnchor {
    std::int32_t x, y;
    std::uint16_t kind;
    std::uint16_t radius;
};

class PcgWorld {
public:
    static constexpr std::int32_t CHUNK_SIZE = 16;
    static constexpr std::int32_t WORLD_HEIGHT = 128;
    explicit PcgWorld(std::uint64_t seed = 4839201ULL) : seed_(seed) {}

    std::uint64_t seed() const { return seed_; }
    SurfaceSample sample_surface(std::int32_t world_x, std::int32_t world_y) const;
    // Retorna a âncora determinística mais próxima que pode afetar esta coluna.
    bool structure_at(std::int32_t world_x, std::int32_t world_y,
                      StructureAnchor& out) const;
    // Resolve one 96x96 macro-cell once, instead of testing every world tile.
    bool structure_anchor_for_cell(std::int32_t cell_x, std::int32_t cell_y,
                                   StructureAnchor& out) const;

private:
    std::uint64_t seed_;
    std::uint32_t hash(std::int32_t x, std::int32_t y,
                       std::uint32_t salt = 0) const;
    float value_noise(float x, float y, std::uint32_t salt) const;
    float fbm(float x, float y, std::uint32_t salt,
              int octaves, float persistence) const;
    float continent_mask(float x, float y) const;
    Biome choose_biome(float temperature, float moisture,
                        float elevation, float continent) const;
};

} // namespace mineplus

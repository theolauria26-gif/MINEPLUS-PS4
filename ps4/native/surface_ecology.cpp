#include "surface_ecology.hpp"

namespace mineplus {
namespace {
constexpr std::uint8_t bit(Biome biome) {
    return static_cast<std::uint8_t>(1u << static_cast<unsigned>(biome));
}
constexpr std::uint8_t woodland = bit(Biome::Forest) | bit(Biome::Plains);
constexpr std::uint8_t dryCold = bit(Biome::Desert) | bit(Biome::Tundra);
constexpr std::uint8_t wildlands = bit(Biome::Highlands) |
    bit(Biome::Volcanic) | bit(Biome::CrystalWastes);
constexpr SurfaceSpecies species[] = {
    {0, "Mossback Grazer", SurfaceBehavior::Grazer, woodland, 105, 255, 45, 220},
    {1, "Briar Stalker", SurfaceBehavior::Predator, bit(Biome::Forest), 90, 255, 55, 230},
    {2, "Glimmer Hare", SurfaceBehavior::Skittish, woodland, 80, 255, 35, 225},
    {3, "Fern Runner", SurfaceBehavior::Skittish, woodland, 110, 255, 40, 210},
    {4, "Dune Grazer", SurfaceBehavior::Grazer, bit(Biome::Desert), 0, 145, 105, 255},
    {5, "Frost Prowler", SurfaceBehavior::Predator, bit(Biome::Tundra), 0, 210, 0, 105},
    {6, "Glasswing Moth", SurfaceBehavior::Skittish, dryCold, 0, 180, 30, 210},
    {7, "Burrow Fox", SurfaceBehavior::Territorial, dryCold, 0, 190, 0, 150},
    {8, "Ashhorn", SurfaceBehavior::Territorial, wildlands, 0, 255, 0, 255},
    {9, "Rift Hound", SurfaceBehavior::Predator, wildlands, 0, 255, 0, 255},
    {10, "Shardback", SurfaceBehavior::Grazer, wildlands, 0, 255, 0, 255},
    {11, "Pale Wisp", SurfaceBehavior::Skittish, wildlands, 0, 255, 0, 255}
};
constexpr std::uint8_t species_count =
    static_cast<std::uint8_t>(sizeof(species) / sizeof(species[0]));
}

const SurfaceSpecies* surface_species(std::uint8_t id) {
    return id < species_count ? &species[id] : nullptr;
}

bool surface_species_can_spawn(std::uint8_t id, Biome biome,
                               std::uint8_t moisture,
                               std::uint8_t temperature,
                               std::uint8_t surface) {
    const SurfaceSpecies* entry = surface_species(id);
    if (!entry || surface == 9u || surface == 13u ||
        biome == Biome::Ocean) return false;
    const std::uint8_t biomeBit = bit(biome);
    return (entry->biome_mask & biomeBit) != 0u &&
           moisture >= entry->min_moisture &&
           moisture <= entry->max_moisture &&
           temperature >= entry->min_temperature &&
           temperature <= entry->max_temperature;
}

std::uint8_t choose_surface_species(Biome biome, std::uint8_t moisture,
                                    std::uint8_t temperature,
                                    std::uint32_t variant) {
    const std::uint8_t first = biome == Biome::Desert || biome == Biome::Tundra
        ? 4u : (biome == Biome::Highlands || biome == Biome::Volcanic ||
                biome == Biome::CrystalWastes ? 8u : 0u);
    const std::uint8_t count = 4u;
    for (std::uint8_t offset = 0; offset < count; ++offset) {
        const std::uint8_t id = static_cast<std::uint8_t>(
            first + ((variant + offset) % count));
        if (surface_species_can_spawn(id, biome, moisture, temperature, 1u))
            return id;
    }
    return 255u;
}

} // namespace mineplus

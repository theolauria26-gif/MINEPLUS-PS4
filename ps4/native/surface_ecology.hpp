#pragma once
#include "pcg_world.hpp"
#include <cstdint>

namespace mineplus {

enum class SurfaceBehavior : std::uint8_t {
    Grazer, Skittish, Predator, Territorial
};

struct SurfaceSpecies {
    std::uint8_t id;
    const char* name;
    SurfaceBehavior behavior;
    std::uint8_t biome_mask;
    std::uint8_t min_moisture;
    std::uint8_t max_moisture;
    std::uint8_t min_temperature;
    std::uint8_t max_temperature;
};

const SurfaceSpecies* surface_species(std::uint8_t id);
bool surface_species_can_spawn(std::uint8_t id, Biome biome,
                               std::uint8_t moisture,
                               std::uint8_t temperature,
                               std::uint8_t surface);
std::uint8_t choose_surface_species(Biome biome, std::uint8_t moisture,
                                    std::uint8_t temperature,
                                    std::uint32_t variant);

} // namespace mineplus

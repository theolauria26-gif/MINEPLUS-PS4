#include "surface_ecology.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using namespace mineplus;
    for (std::uint8_t id = 0; id < 12; ++id) {
        const auto* entry = surface_species(id);
        assert(entry && entry->id == id && entry->name[0] != '\0');
    }
    assert(surface_species(12) == nullptr);
    assert(surface_species(255) == nullptr);

    const auto forest = choose_surface_species(Biome::Forest, 190, 150, 0);
    const auto plains = choose_surface_species(Biome::Plains, 180, 150, 3);
    const auto desert = choose_surface_species(Biome::Desert, 80, 190, 0);
    const auto tundra = choose_surface_species(Biome::Tundra, 80, 65, 1);
    const auto highlands = choose_surface_species(Biome::Highlands, 120, 120, 2);
    assert(forest < 4);
    assert(plains < 4);
    assert(desert >= 4 && desert < 8);
    assert(tundra >= 4 && tundra < 8);
    assert(highlands >= 8 && highlands < 12);
    assert(choose_surface_species(Biome::Ocean, 120, 120, 0) == 255u);

    for (unsigned seed = 0; seed < 64; ++seed) {
        const auto a = choose_surface_species(Biome::Forest, 190, 150, seed);
        const auto b = choose_surface_species(Biome::Forest, 190, 150, seed);
        assert(a == b);
        assert(a < 4);
    }

    assert(!surface_species_can_spawn(0, Biome::Forest, 190, 150, 13));
    assert(!surface_species_can_spawn(0, Biome::Forest, 190, 150, 9));
    assert(!surface_species_can_spawn(0, Biome::Ocean, 190, 150, 1));
    assert(!surface_species_can_spawn(1, Biome::Plains, 190, 150, 1));
    assert(!surface_species_can_spawn(4, Biome::Desert, 200, 190, 1));
    assert(!surface_species_can_spawn(5, Biome::Tundra, 80, 180, 1));
    assert(surface_species_can_spawn(0, Biome::Forest, 190, 150, 1));
    assert(surface_species_can_spawn(5, Biome::Tundra, 80, 65, 1));

    assert(surface_species(0)->behavior == SurfaceBehavior::Grazer);
    assert(surface_species(1)->behavior == SurfaceBehavior::Predator);
    assert(surface_species(2)->behavior == SurfaceBehavior::Skittish);
    assert(surface_species(7)->behavior == SurfaceBehavior::Territorial);
    assert(surface_species(10)->behavior == SurfaceBehavior::Grazer);
    assert(surface_species(11)->behavior == SurfaceBehavior::Skittish);

    std::cout << "MINEPLUS surface ecology tests: OK\n";
    return 0;
}

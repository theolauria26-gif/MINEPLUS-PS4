#include "pcg_world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <climits>

namespace mineplus {
namespace {
inline float lerp(float a, float b, float t) { return a + (b-a)*t; }
inline float smooth(float t) { return t*t*(3.0f-2.0f*t); }
inline std::int32_t floor_i(float v) { return static_cast<std::int32_t>(std::floor(v)); }
inline std::int32_t floor_div4(std::int32_t v) {
    const std::int32_t q = v / 4;
    const std::int32_t r = v % 4;
    return (r < 0) ? q - 1 : q;
}
}

std::uint32_t PcgWorld::hash(std::int32_t x, std::int32_t y, std::uint32_t salt) const {
    std::uint64_t n = seed_ ^ (std::uint64_t(std::uint32_t(x))*0x9E3779B185EBCA87ULL)
                            ^ (std::uint64_t(std::uint32_t(y))*0xC2B2AE3D27D4EB4FULL)
                            ^ (std::uint64_t(salt)*0x165667B19E3779F9ULL);
    n ^= n >> 30; n *= 0xBF58476D1CE4E5B9ULL;
    n ^= n >> 27; n *= 0x94D049BB133111EBULL;
    n ^= n >> 31;
    return static_cast<std::uint32_t>(n >> 16);
}

float PcgWorld::value_noise(float x, float y, std::uint32_t salt) const {
    const int ix=floor_i(x), iy=floor_i(y);
    const float fx=smooth(x-ix), fy=smooth(y-iy);
    auto v=[&](int a,int b){ return float(hash(a,b,salt)&0xFFFFu)/32767.5f-1.0f; };
    return lerp(lerp(v(ix,iy),v(ix+1,iy),fx),
                lerp(v(ix,iy+1),v(ix+1,iy+1),fx),fy);
}

float PcgWorld::fbm(float x,float y,std::uint32_t salt,int octaves,float persistence) const {
    float sum=0, amplitude=1, norm=0;
    for(int i=0;i<octaves;++i){
        sum += value_noise(x,y,salt+std::uint32_t(i)*1013u)*amplitude;
        norm += amplitude; amplitude *= persistence; x *= 2.03f; y *= 2.03f;
    }
    return norm>0 ? sum/norm : 0;
}

float PcgWorld::continent_mask(float x,float y) const {
    // Continente central + faixa oceânica; fora dela os macro-biomas continuam sem fim.
    const float radius=4200.0f;
    const float distance=std::sqrt(x*x+y*y);
    const float edge=std::max(0.0f,std::min(1.0f,(distance-radius)/900.0f));
    return 1.0f-edge;
}

Biome PcgWorld::choose_biome(float temp,float wet,float elevation,float continent) const {
    if(continent<0.18f && elevation<0.05f) return Biome::Ocean;
    if(elevation>0.66f) return Biome::Highlands;
    if(temp< -0.34f) return Biome::Tundra;
    if(temp>0.42f && wet< -0.12f) return Biome::Desert;
    if(temp>0.30f && wet>0.35f) return Biome::Volcanic;
    if(wet< -0.42f) return Biome::CrystalWastes;
    if(wet>0.05f) return Biome::Forest;
    return Biome::Plains;
}

SurfaceSample PcgWorld::sample_surface(std::int32_t x,std::int32_t y) const {
    const float fx=float(x), fy=float(y);
    const float broad=fbm(fx/1500.0f,fy/1500.0f,11,5,0.52f);
    const float ridge=1.0f-std::fabs(fbm(fx/410.0f,fy/410.0f,29,4,0.56f));
    const float detail=fbm(fx/75.0f,fy/75.0f,47,3,0.48f);
    const float continent=continent_mask(fx,fy);
    // Broad ridged fields create connected mountain chains instead of isolated
    // height spikes. A separate narrow field carves winding lowland valleys.
    const float mountainSignal=1.0f-std::fabs(
        fbm(fx/760.0f,fy/760.0f,0x4D4F554Eu,4,0.57f));
    const float mountainMask=smooth(std::clamp(
        (mountainSignal-0.61f)/0.25f,0.0f,1.0f));
    const float valleySignal=std::fabs(
        fbm(fx/520.0f,fy/520.0f,0x56414C4Cu,3,0.55f));
    const float valleyMask=1.0f-smooth(std::clamp(
        valleySignal/0.19f,0.0f,1.0f));
    const float elevation=broad*0.34f+ridge*0.42f+detail*0.12f+
        continent*0.18f-0.32f+mountainMask*0.30f-valleyMask*0.13f;
    const float latitudeCooling=static_cast<float>(std::fabs(fy)/12000.0f);
    const float temperature=fbm(fx/2300.0f,fy/2300.0f,71,3,0.5f)-std::min(0.8f,latitudeCooling);
    const float moisture=fbm(fx/1250.0f,fy/1250.0f,97,4,0.53f);
    const Biome biome=choose_biome(temperature,moisture,elevation,continent);
    int height=static_cast<int>(34.0f+elevation*58.0f+mountainMask*16.0f);
    std::uint8_t tile=1; // grass
    switch(biome){
        case Biome::Ocean: height=18+int((detail+1.0f)*3.0f); tile=13; break; // water
        case Biome::Plains: tile=1; break;
        case Biome::Forest: tile=5; break; // moss/forest floor
        case Biome::Desert: tile=9; height-=3; break;
        case Biome::Tundra: tile=10; break;
        case Biome::Highlands: tile=3; height+=12; break;
        case Biome::Volcanic: tile=16; height+=5; break;
        case Biome::CrystalWastes: tile=6; break;
    }
    // A narrow continental shelf transition creates beaches along the
    // inland edge of the ocean mask, without turning the desert biome into
    // a universal coastline. Cold regions retain snow/ice rather than sand.
    const bool coastalBeach = continent >= 0.18f && continent < 0.34f &&
        elevation < 0.20f && temperature > -0.58f &&
        biome != Biome::Desert && biome != Biome::Volcanic &&
        biome != Biome::CrystalWastes;
    if (coastalBeach) {
        tile = 9;
        height = std::max(20, height - 1);
    }
    // Small inland lakes are placed from a global macro-cell grid. Each
    // cell derives one stable basin center and radius from the world seed,
    // so lakes remain identical across chunk borders and save/reload cycles.
    // The irregular shoreline comes from the surrounding biome/elevation
    // fields; a shallow lowered basin keeps water visually distinct.
    const std::int32_t lakeCellX = floor_i(fx / 420.0f);
    const std::int32_t lakeCellY = floor_i(fy / 420.0f);
    bool lakeWater = false;
    bool lakeShore = false;
    const bool lakeEligible = continent >= 0.18f && elevation > -0.02f &&
        temperature > -0.82f && biome != Biome::Ocean &&
        biome != Biome::Desert && biome != Biome::Volcanic &&
        biome != Biome::CrystalWastes;
    if (lakeEligible) {
        for (std::int32_t cy = lakeCellY - 1; cy <= lakeCellY + 1; ++cy) {
            for (std::int32_t cx = lakeCellX - 1; cx <= lakeCellX + 1; ++cx) {
                const std::uint32_t basin = hash(cx, cy, 0x1A4E5101u);
                const std::int32_t centerX = cx * 420 + 72 +
                    static_cast<std::int32_t>(basin % 276u);
                const std::int32_t centerY = cy * 420 + 72 +
                    static_cast<std::int32_t>((basin >> 9) % 276u);
                const std::int32_t radius = 17 +
                    static_cast<std::int32_t>((basin >> 18) % 25u);
                const std::int32_t dx = static_cast<std::int32_t>(x - centerX);
                const std::int32_t dy = static_cast<std::int32_t>(y - centerY);
                // A mild anisotropy avoids perfectly circular ponds.
                const std::int64_t basinDistance =
                    std::int64_t(dx) * dx * 4 +
                    std::int64_t(dy) * dy * 3;
                const std::int64_t waterLimit =
                    std::int64_t(radius) * radius * 3;
                const std::int64_t shoreLimit =
                    std::int64_t(radius + 7) * (radius + 7) * 3;
                if (basinDistance <= waterLimit) lakeWater = true;
                else if (basinDistance <= shoreLimit) lakeShore = true;
            }
        }
    }
    if (lakeWater) {
        tile = 13; // Inland lake water.
        height = std::max(19, height - 2);
    } else if (lakeShore && (tile == 1 || tile == 5 || tile == 3 || tile == 10)) {
        tile = 9; // Sandy or pale shoreline transition.
        height = std::max(20, height - 1);
    }

    // Meandering surface rivers use a separate low-frequency field, so their
    // banks remain continuous across chunk boundaries and world reloads.
    // Rivers favor moist temperate land and avoid deserts, volcanic fields,
    // crystal wastes and the outer ocean shelf.
    const float riverField = std::fabs(fbm(fx/210.0f, fy/210.0f, 131, 3, 0.54f));
    const bool riverEligible = continent >= 0.18f && elevation > 0.03f &&
        temperature > -0.72f && moisture > -0.35f &&
        biome != Biome::Desert && biome != Biome::Volcanic &&
        biome != Biome::CrystalWastes && biome != Biome::Ocean;
    if (riverEligible && riverField < 0.018f) {
        tile = 13; // Water: a narrow channel cut into the local terrain.
        height = std::max(19, height - 3);
    } else if (riverEligible && riverField < 0.043f &&
               (tile == 1 || tile == 5 || tile == 3)) {
        tile = 9; // Sand banks transition into grass, forest and highlands.
        height = std::max(20, height - 1);
    }
    height=std::max(1,std::min(WORLD_HEIGHT-1,height));
    return {static_cast<std::int16_t>(height),tile,
            static_cast<std::uint8_t>(biome),
            static_cast<std::uint8_t>(std::clamp(int((moisture+1.0f)*127.5f),0,255)),
            static_cast<std::uint8_t>(std::clamp(int((temperature+1.0f)*127.5f),0,255))};
}


bool PcgWorld::surface_tree_at(std::int32_t x, std::int32_t y,
                               const SurfaceSample& column) const {
    const Biome biome = static_cast<Biome>(column.biome);
    if (column.surface == 9 || column.surface == 13 ||
        biome == Biome::Ocean || biome == Biome::Desert ||
        biome == Biome::Volcanic || biome == Biome::CrystalWastes)
        return false;

    // Low-frequency grove density creates broad wooded patches and clearings;
    // a separate coordinate hash distributes individual trunks inside them.
    const float grove = fbm(float(x) / 48.0f, float(y) / 48.0f,
                            0x7AEE5101u, 2, 0.58f);
    const std::uint32_t placement = hash(x, y, 0x7AEE5102u) % 100u;
    if (biome == Biome::Forest)
        return grove > -0.24f && placement < 24u;
    if (biome == Biome::Plains && column.moisture >= 150u)
        return grove > 0.28f && placement < 7u;
    // Cold-climate conifers are sparse and favor sheltered, moist pockets.
    if (biome == Biome::Tundra && column.moisture >= 112u)
        return grove > 0.02f && placement < 9u;
    // Alpine trees cling to damp highland ledges rather than covering bare peaks.
    if (biome == Biome::Highlands && column.moisture >= 126u)
        return grove > 0.10f && placement < 11u;
    return false;
}


VoxelSample PcgWorld::sample_voxel(std::int32_t x, std::int32_t y,
                                    std::int32_t z) const {
    if (z < 0 || z >= WORLD_HEIGHT) return {};
    const SurfaceSample column = sample_surface(x, y);
    const std::int32_t surface_z = column.height;
    const Biome biome = static_cast<Biome>(column.biome);

    // Major landmarks can conceal a deterministic five-chamber primordial
    // complex. Rooms and connecting corridors are carved below the surface,
    // while the roof/floor remain solid. Geometry uses seed + global anchor.
    StructureAnchor dungeon{};
    const std::int32_t depth = surface_z - z;
    if (surface_z >= 15 && depth >= 8 && depth <= 14 &&
        structure_at(x, y, dungeon)) {
        const std::int32_t dx = x - dungeon.x;
        const std::int32_t dy = y - dungeon.y;
        // The anchor kind selects one of four room profiles. All profiles
        // retain the five-room topology, but vary chamber proportions and
        // corridor width so exploration landmarks do not feel interchangeable.
        const std::uint16_t profile = static_cast<std::uint16_t>(dungeon.kind % 4u);
        const std::int32_t main_divisor = profile == 1u ? 3 : (profile == 2u ? 5 : 4);
        const std::int32_t wing_divisor = profile == 2u ? 6 : (profile == 3u ? 4 : 5);
        const std::int32_t offset_numerator = profile == 1u ? 3 : (profile == 2u ? 7 : 2);
        const std::int32_t offset_denominator = profile == 1u ? 5 : (profile == 2u ? 10 : 3);
        const std::int32_t main_radius = std::max(2, int(dungeon.radius) / main_divisor);
        const std::int32_t side_radius = std::max(2, int(dungeon.radius) / wing_divisor);
        const std::int32_t offset = std::max(3, int(dungeon.radius) *
                                                offset_numerator / offset_denominator);
        const std::int32_t corridor_half_width = profile == 3u ? 2 : 1;
        const std::int64_t main_d2 = std::int64_t(dx) * dx + std::int64_t(dy) * dy;
        // Five chambers (central + four cardinal wings) form a cross-shaped
        // primordial complex. The orthogonal corridors remain inside the anchor
        // radius, so neighboring chunks derive exactly the same connected layout.
        const std::int64_t side2 = std::int64_t(side_radius) * side_radius;
        const std::int64_t north = std::int64_t(dx) * dx +
            std::int64_t(dy - offset) * (dy - offset);
        const std::int64_t south = std::int64_t(dx) * dx +
            std::int64_t(dy + offset) * (dy + offset);
        const std::int64_t west = std::int64_t(dx - offset) * (dx - offset) +
            std::int64_t(dy) * dy;
        const std::int64_t east = std::int64_t(dx + offset) * (dx + offset) +
            std::int64_t(dy) * dy;
        const bool chamber = main_d2 <= std::int64_t(main_radius) * main_radius ||
            north <= side2 || south <= side2 || west <= side2 || east <= side2;
        const bool corridor = (std::abs(dx) <= corridor_half_width && std::abs(dy) <= offset) ||
                              (std::abs(dy) <= corridor_half_width && std::abs(dx) <= offset);
        const bool inside = chamber || corridor;
        if (inside && (depth == 8 || depth == 14))
            return {VoxelMaterial::DarkStone, true, false};
        if (inside && depth >= 9 && depth <= 13) {
            // Four compact rune pylons mark the central chamber's corners.
            // The ember cap and rune core are solid, mineable landmarks rather
            // than decorative data that would require a separate entity system.
            const std::int32_t pylon_offset = std::max(1, main_radius / 2);
            const bool pylon = std::abs(dx) == pylon_offset &&
                               std::abs(dy) == pylon_offset;
            if (pylon && depth == 11)
                return {VoxelMaterial::Rune, true, false};
            if (pylon && depth == 10)
                return {VoxelMaterial::Ember, true, false};

            // Each cardinal wing receives a distinct landmark at its center.
            // These compact formations make the five-room complex readable:
            // crystal growth, rune altar, obsidian monolith and bone relic.
            if (dx == 0 && dy == -offset) {
                if (depth == 11 || depth == 12)
                    return {VoxelMaterial::Crystal, true, false};
            } else if (dx == 0 && dy == offset) {
                if (depth == 11)
                    return {VoxelMaterial::Rune, true, false};
                if (depth == 10)
                    return {VoxelMaterial::Ember, true, false};
            } else if (dx == -offset && dy == 0) {
                if (depth == 11)
                    return {VoxelMaterial::Obsidian, true, false};
                if (depth == 10)
                    return {VoxelMaterial::DarkStone, true, false};
            } else if (dx == offset && dy == 0) {
                if (depth == 11)
                    return {VoxelMaterial::Bone, true, false};
                if (depth == 10)
                    return {VoxelMaterial::Crystal, true, false};
            }

            // Sparse crystal seams make exploration and mining worthwhile without
            // filling the walkable chambers or changing the outer cave silhouette.
            // Floor-level crystal seams form small 2x2 deposits instead of
            // isolated floating blocks. Their macro-cell hash keeps clusters
            // stable across chunk boundaries and negative world coordinates.
            if (depth == 13) {
                // Every chamber has one guaranteed, mineable seam on the
                // central room's eastern floor edge.
                if (dx == main_radius - 1 && dy == 0)
                    return {VoxelMaterial::Crystal, true, false};
                const std::int32_t vein_x = floor_div4(x);
                const std::int32_t vein_y = floor_div4(y);
                const std::uint32_t vein = hash(vein_x, vein_y, 0xC4A7E101u);
                const std::int32_t local_x = x - vein_x * 4;
                const std::int32_t local_y = y - vein_y * 4;
                if ((vein % 29u) == 0u && local_x < 2 && local_y < 2)
                    return {VoxelMaterial::Crystal, true, false};
            }
            // Rare wall crystals provide vertical points of interest without
            // occupying the central walking volume.
            if (depth == 11 && (hash(x, y, 0xC4A7E001u) % 43u) == 0u)
                return {VoxelMaterial::Crystal, true, false};
            return {};
        }
    }

    // Ocean columns expose water up to a stable sea level.
    if (z > surface_z) {
        if (biome == Biome::Ocean && z <= 22)
            return {VoxelMaterial::Water, false, true};
        return {};
    }
    if (z == surface_z) {
        // Surface overrides represent actual river water and its sandy banks;
        // retain the biome identity independently for temperature/moisture.
        if (column.surface == 13)
            return {VoxelMaterial::Water, false, true};
        if (column.surface == 9 && biome != Biome::Desert &&
            biome != Biome::Ocean)
            return {VoxelMaterial::Sand, true, false};
        VoxelMaterial top = VoxelMaterial::Grass;
        switch (biome) {
            case Biome::Ocean: top = VoxelMaterial::Sand; break;
            case Biome::Forest: top = VoxelMaterial::Moss; break;
            case Biome::Desert: top = VoxelMaterial::Sand; break;
            case Biome::Tundra: top = VoxelMaterial::Snow; break;
            case Biome::Highlands: top = VoxelMaterial::Stone; break;
            case Biome::Volcanic: top = VoxelMaterial::VolcanicRock; break;
            case Biome::CrystalWastes: top = VoxelMaterial::Crystal; break;
            default: break;
        }
        return {top, true, false};
    }

    if (depth <= 3) {
        const VoxelMaterial soil = (biome == Biome::Desert || biome == Biome::Ocean)
            ? VoxelMaterial::Sand : VoxelMaterial::Dirt;
        return {soil, true, false};
    }
    if (z > 4) {
        // Floor division keeps cave cells aligned across negative coordinates.
        const std::int32_t cave_x = floor_div4(x);
        const std::int32_t cave_y = floor_div4(y);
        const std::uint32_t cave = hash(cave_x, cave_y,
            0xCA000000u ^ static_cast<std::uint32_t>(z / 3));
        if ((cave % 100u) < 13u) return {};
        const std::uint32_t ore = hash(x, y, 0x0E000000u ^ static_cast<std::uint32_t>(z));
        if ((ore % 211u) == 0u) return {VoxelMaterial::Ore, true, false};
    }
    return {VoxelMaterial::Stone, true, false};
}

bool PcgWorld::structure_anchor_for_cell(std::int32_t ax,std::int32_t ay,
                                                StructureAnchor& out) const {
    constexpr std::int32_t CELL=96;
    const auto h=hash(ax,ay,313);
    if(h%11u!=0u) return false;
    const std::int64_t sx=std::int64_t(ax)*CELL+std::int64_t(hash(ax,ay,317)%CELL);
    const std::int64_t sy=std::int64_t(ay)*CELL+std::int64_t(hash(ax,ay,331)%CELL);
    if(sx<INT32_MIN||sx>INT32_MAX||sy<INT32_MIN||sy>INT32_MAX) return false;
    const std::int32_t radius=7+std::int32_t(hash(ax,ay,337)%18);
    // Six landmark families: standing stones, shrines, crystal groves,
    // bone altars, surface camps and ruined frontier watch-posts.
    out={static_cast<std::int32_t>(sx),static_cast<std::int32_t>(sy),
         static_cast<std::uint16_t>(1+hash(ax,ay,347)%6),
         static_cast<std::uint16_t>(radius)};
    return true;
}

bool PcgWorld::surface_road_link_for_cell(std::int32_t cell_x,
                                                   std::int32_t cell_y,
                                                   StructureAnchor& from,
                                                   StructureAnchor& to) const {
    if (!structure_anchor_for_cell(cell_x, cell_y, from)) return false;
    constexpr std::int64_t MAX_DISTANCE_SQUARED = 420LL * 420LL;
    std::int64_t best_distance = MAX_DISTANCE_SQUARED + 1;
    bool found = false;
    for (std::int32_t oy = -4; oy <= 4; ++oy) {
        for (std::int32_t ox = -4; ox <= 4; ++ox) {
            if (ox == 0 && oy == 0) continue;
            StructureAnchor candidate{};
            if (!structure_anchor_for_cell(cell_x + ox, cell_y + oy, candidate))
                continue;
            const std::int64_t dx = std::int64_t(candidate.x) - from.x;
            const std::int64_t dy = std::int64_t(candidate.y) - from.y;
            const std::int64_t distance = dx * dx + dy * dy;
            if (distance > MAX_DISTANCE_SQUARED) continue;
            if (!found || distance < best_distance ||
                (distance == best_distance &&
                 (candidate.y < to.y ||
                  (candidate.y == to.y && candidate.x < to.x)))) {
                to = candidate;
                best_distance = distance;
                found = true;
            }
        }
    }
    return found;
}

bool PcgWorld::structure_at(std::int32_t x,std::int32_t y,StructureAnchor& out) const {
    // Query neighboring macro-cells because a structure can cross chunk borders.
    constexpr int CELL=96;
    const int cx=x/CELL-((x%CELL)!=0&&x<0?1:0);
    const int cy=y/CELL-((y%CELL)!=0&&y<0?1:0);
    bool found=false; std::uint64_t best=~std::uint64_t(0);
    for(int oy=-1;oy<=1;++oy) for(int ox=-1;ox<=1;++ox){
        StructureAnchor candidate{};
        if(!structure_anchor_for_cell(cx+ox,cy+oy,candidate)) continue;
        const std::int64_t dx=std::int64_t(x)-candidate.x;
        const std::int64_t dy=std::int64_t(y)-candidate.y;
        if(dx*dx+dy*dy>std::int64_t(candidate.radius)*candidate.radius) continue;
        const std::uint64_t score=std::uint64_t(dx*dx+dy*dy);
        if(score<best){best=score;out=candidate;found=true;}
    }
    return found;
}
} // namespace mineplus

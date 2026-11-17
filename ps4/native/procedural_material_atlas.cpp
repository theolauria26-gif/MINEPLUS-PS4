#include "procedural_material_atlas.hpp"
#include <array>
namespace mineplus {
namespace {
struct Palette { std::uint8_t base[3], shade[3], accent[3]; };
constexpr std::array<Palette, MATERIAL_ATLAS_LAST_ID + 1> palettes{{
    {},
    {{74,151,65},{49,111,51},{133,190,78}},       // grass
    {{126,83,55},{91,58,43},{158,111,72}},        // dirt
    {{119,126,135},{78,86,98},{164,171,178}},     // stone
    {{132,91,54},{91,59,38},{174,126,75}},        // wood
    {{77,111,76},{48,78,62},{119,147,91}},        // moss
    {{74,169,185},{37,112,145},{156,229,223}},    // crystal
    {{190,190,164},{126,126,115},{226,219,188}},  // bone
    {{59,61,77},{35,37,52},{103,94,119}},         // dark stone
    {{205,177,111},{164,132,76},{232,211,151}},   // sand
    {{211,225,235},{157,185,207},{244,249,255}},  // snow
    {{117,190,208},{73,137,169},{185,229,237}},   // ice
    {{204,76,31},{126,39,27},{255,155,48}},       // lava
    {{43,115,157},{29,75,125},{94,181,202}},      // water
    {{116,119,128},{73,77,88},{220,151,78}},      // iron ore
    {{61,43,76},{35,28,52},{119,70,125}},         // obsidian
    {{168,68,35},{105,42,31},{255,145,45}},       // ember
    {{84,67,139},{48,43,91},{172,139,236}},       // rune
    {{183,190,198},{112,124,140},{225,203,157}},  // iron bar
    {{49,38,68},{27,25,45},{129,79,173}},         // obsidian core
    {{204,91,44},{120,48,37},{255,190,73}}        // ember catalyst
}};
std::uint32_t noise(unsigned x, unsigned y, unsigned material) {
    std::uint32_t n=x*0x9E3779B9u ^ y*0x85EBCA6Bu ^ material*0xC2B2AE35u;
    n^=n>>16; n*=0x7FEB352Du; n^=n>>15; n*=0x846CA68Bu; return n^(n>>16);
}
}
bool build_procedural_material_atlas(std::vector<std::uint8_t>& rgba) {
    constexpr std::size_t bytes=static_cast<std::size_t>(MATERIAL_ATLAS_WIDTH)*
        MATERIAL_ATLAS_HEIGHT*4u;
    rgba.assign(bytes,0u);
    for(unsigned id=MATERIAL_ATLAS_FIRST_ID;id<=MATERIAL_ATLAS_LAST_ID;++id) {
        const Palette& p=palettes[id];
        if(id>=palettes.size()) { rgba.clear(); return false; }
        MaterialAtlasRegion region{};
        if(!material_atlas_region(static_cast<std::uint8_t>(id),region)) {
            rgba.clear(); return false;
        }
        for(unsigned y=0;y<MATERIAL_ATLAS_TILE_SIZE;++y) {
            for(unsigned x=0;x<MATERIAL_ATLAS_TILE_SIZE;++x) {
                const std::uint32_t h=noise(x,y,id);
                const unsigned selector=(h>>27)&7u;
                const bool vein=((x*3u+y*5u+id)%11u)==0u;
                const std::uint8_t* color=selector==0u? p.shade :
                    (vein||selector==7u? p.accent:p.base);
                // Carved/rune materials receive a restrained repeating motif.
                const bool motif=(id==17u||id==19u) &&
                    ((x==y)||(x+y==MATERIAL_ATLAS_TILE_SIZE-1u));
                const unsigned px=region.tile_x*MATERIAL_ATLAS_TILE_SIZE+x;
                const unsigned py=region.tile_y*MATERIAL_ATLAS_TILE_SIZE+y;
                const std::size_t at=(static_cast<std::size_t>(py)*MATERIAL_ATLAS_WIDTH+px)*4u;
                for(unsigned c=0;c<3;++c)
                    rgba[at+c]=motif?p.accent[c]:color[c];
                rgba[at+3]=255u;
            }
        }
    }
    return true;
}
} // namespace mineplus

#include "procedural_material_atlas.hpp"
#include <array>
#include <cstdint>
namespace mineplus {
namespace {
struct Palette { std::uint8_t base[3], shade[3], accent[3], highlight[3]; };
constexpr std::array<Palette, MATERIAL_ATLAS_LAST_ID + 1> palettes{{
    {},
    {{74,151,65},{38,91,48},{111,174,68},{166,205,105}},       // grass
    {{126,83,55},{79,49,39},{158,111,72},{190,145,99}},        // dirt
    {{119,126,135},{67,75,87},{164,171,178},{205,211,216}},    // stone
    {{132,91,54},{76,48,34},{174,126,75},{207,158,101}},       // wood
    {{77,111,76},{39,70,56},{119,147,91},{158,180,111}},       // moss
    {{74,169,185},{30,91,125},{156,229,223},{216,250,244}},    // crystal
    {{190,190,164},{118,119,111},{226,219,188},{246,239,211}}, // bone
    {{59,61,77},{27,29,44},{103,94,119},{151,133,169}},        // dark stone
    {{205,177,111},{154,119,69},{232,211,151},{248,231,183}},  // sand
    {{211,225,235},{151,178,198},{244,249,255},{255,255,255}}, // snow
    {{117,190,208},{53,119,153},{185,229,237},{223,250,255}},  // ice
    {{204,76,31},{105,35,25},{255,155,48},{255,218,105}},      // lava
    {{43,115,157},{20,63,108},{94,181,202},{164,226,231}},     // water
    {{116,119,128},{59,65,78},{220,151,78},{249,203,119}},     // iron ore
    {{61,43,76},{25,22,42},{119,70,125},{183,132,198}},        // obsidian
    {{168,68,35},{91,34,27},{255,145,45},{255,209,91}},        // ember
    {{84,67,139},{39,34,78},{172,139,236},{225,207,255}},      // rune
    {{183,190,198},{91,105,121},{225,203,157},{249,242,215}},  // iron bar
    {{49,38,68},{17,16,31},{129,79,173},{203,157,244}},        // obsidian core
    {{204,91,44},{102,38,35},{255,190,73},{255,235,155}},      // ember catalyst
    {{92,72,151},{35,29,76},{174,145,244},{229,215,255}},      // ancient rune
    {{105,58,48},{48,31,39},{174,75,48},{244,139,66}},         // volcanic rock
    {{95,104,116},{43,51,65},{174,119,73},{239,194,116}}      // ore
}};
std::uint32_t noise(unsigned x, unsigned y, unsigned material) {
    std::uint32_t n=x*0x9E3779B9u ^ y*0x85EBCA6Bu ^ material*0xC2B2AE35u;
    n^=n>>16; n*=0x7FEB352Du; n^=n>>15; n*=0x846CA68Bu; return n^(n>>16);
}
unsigned pattern(unsigned id, unsigned x, unsigned y, std::uint32_t h) {
    const unsigned n=(h>>28)&15u;
    switch(id) {
    case 1: return y<5u ? (n<5u?3u:2u) : ((n<3u)?0u:1u); // grass blades and soil flecks
    case 2: return n<3u?0u:(n==3u?3u:1u); // granular earth
    case 3: return ((x+y*3u+id)%19u==0u)?0u:(n<3u?0u:(n==3u?3u:1u)); // fractured stone
    case 4: return ((x+(h&3u))%7u==0u)?3u:((n<3u)?0u:1u); // timber grain
    case 5: return ((x*7u+y*11u+(h&7u))%13u<4u)?2u:(n<4u?3u:1u); // moss clusters
    case 6: return ((x+y)%9u<2u)?3u:(((x*3u+y*5u)%11u<3u)?2u:1u); // crystal facets
    case 7: return (y%9u==0u)?3u:(n<3u?0u:1u); // bone striation
    case 8: return ((x*5u+y*3u)%23u==0u)?2u:(n<3u?0u:1u); // dark masonry seams
    case 9: return (y%6u==0u)?3u:(n<4u?0u:1u); // sand ripples
    case 10: return (n==0u||((x*13u+y*7u)%29u==0u))?3u:1u; // snow crystals
    case 11: return ((x+y*2u)%17u==0u)?3u:(n<4u?2u:1u); // ice fractures
    case 12: return ((x*3u+y*5u)%13u<3u)?3u:(n<5u?2u:1u); // molten channels
    case 13: return ((y+(x/4u))%8u<2u)?3u:(n<3u?2u:1u); // water ripples
    case 14: return ((x*7u+y*11u)%17u<3u)?3u:(n<5u?0u:1u); // ore inclusions
    case 15: return ((x+y*2u)%11u<2u)?3u:(n<4u?2u:1u); // obsidian facets
    case 16: return ((x*5u+y*7u)%13u<3u)?3u:(n<5u?2u:1u); // ember fissures
    case 17: return ((x%8u==4u)||(y%8u==4u))?3u:((n<3u)?2u:1u);
    case 18: return (x%8u<2u||y%8u<2u)?3u:(n<3u?0u:1u); // forged metal
    case 19: return ((x+y)%7u<2u)?3u:(((x/4u+y/4u)%2u)?2u:1u);
    case 20: return ((x*3u+y*3u)%16u<3u)?3u:(n<5u?2u:1u);
    case 21: return ((x==y)||(x+y==MATERIAL_ATLAS_TILE_SIZE-1u)||
                     (x%8u==0u&&y%8u==0u))?3u:((n<3u)?2u:1u);
    case 22: return ((x*5u+y*3u)%11u<3u)?3u:(n<5u?2u:1u); // volcanic seams
    case 23: return ((x*7u+y*11u)%17u<4u)?3u:(n<5u?0u:1u); // ore grains
    default: return 1u;
    }
}
}
bool build_procedural_material_atlas(std::vector<std::uint8_t>& rgba) {
    constexpr std::size_t bytes=static_cast<std::size_t>(MATERIAL_ATLAS_WIDTH)*
        MATERIAL_ATLAS_HEIGHT*4u;
    rgba.assign(bytes,0u);
    for(unsigned id=MATERIAL_ATLAS_FIRST_ID;id<=MATERIAL_ATLAS_LAST_ID;++id) {
        if(id>=palettes.size()) { rgba.clear(); return false; }
        const Palette& p=palettes[id];
        MaterialAtlasRegion region{};
        if(!material_atlas_region(static_cast<std::uint8_t>(id),region)) {
            rgba.clear(); return false;
        }
        for(unsigned y=0;y<MATERIAL_ATLAS_TILE_SIZE;++y) {
            for(unsigned x=0;x<MATERIAL_ATLAS_TILE_SIZE;++x) {
                const std::uint32_t h=noise(x,y,id);
                const unsigned tone=pattern(id,x,y,h);
                const std::uint8_t* selected=tone==0u?p.shade:
                    (tone==2u?p.accent:(tone==3u?p.highlight:p.base));
                // A subtle upper-left bevel gives each pixel tile more readable volume.
                const unsigned edge=(x==0u||y==0u)?1u:((x+1u==MATERIAL_ATLAS_TILE_SIZE||
                    y+1u==MATERIAL_ATLAS_TILE_SIZE)?2u:0u);
                const unsigned px=region.tile_x*MATERIAL_ATLAS_TILE_SIZE+x;
                const unsigned py=region.tile_y*MATERIAL_ATLAS_TILE_SIZE+y;
                const std::size_t at=(static_cast<std::size_t>(py)*MATERIAL_ATLAS_WIDTH+px)*4u;
                for(unsigned c=0;c<3;++c) {
                    unsigned value=selected[c];
                    if(edge==1u)value=(value*112u)/128u;
                    else if(edge==2u)value=(value*104u)/128u;
                    rgba[at+c]=static_cast<std::uint8_t>(value);
                }
                rgba[at+3]=255u;
            }
        }
    }
    return true;
}
} // namespace mineplus

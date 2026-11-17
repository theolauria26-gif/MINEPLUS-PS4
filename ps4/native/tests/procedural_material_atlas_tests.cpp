#include "procedural_material_atlas.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace mineplus;
int main() {
    std::vector<std::uint8_t> first, second;
    assert(build_procedural_material_atlas(first));
    assert(build_procedural_material_atlas(second));
    const std::size_t expected=static_cast<std::size_t>(MATERIAL_ATLAS_WIDTH)*MATERIAL_ATLAS_HEIGHT*4u;
    assert(first.size()==expected && second.size()==expected);
    assert(first==second); // deterministic across runs
    bool transparent_unused=false, opaque_used=false;
    for(unsigned y=0;y<MATERIAL_ATLAS_HEIGHT;++y) for(unsigned x=0;x<MATERIAL_ATLAS_WIDTH;++x) {
        const unsigned id=(y/MATERIAL_ATLAS_TILE_SIZE)*MATERIAL_ATLAS_COLUMNS+
                          (x/MATERIAL_ATLAS_TILE_SIZE)+MATERIAL_ATLAS_FIRST_ID;
        const std::size_t at=(static_cast<std::size_t>(y)*MATERIAL_ATLAS_WIDTH+x)*4u;
        if(id<=MATERIAL_ATLAS_LAST_ID) {
            assert(first[at+3]==255u); opaque_used=true;
        } else {
            assert(first[at]==0u && first[at+1]==0u && first[at+2]==0u && first[at+3]==0u);
            transparent_unused=true;
        }
    }
    assert(opaque_used && transparent_unused);
    // Every material must contain more than a flat two-color fill.
    for(unsigned id=MATERIAL_ATLAS_FIRST_ID;id<=MATERIAL_ATLAS_LAST_ID;++id) {
        const unsigned tx=(id-MATERIAL_ATLAS_FIRST_ID)%MATERIAL_ATLAS_COLUMNS;
        const unsigned ty=(id-MATERIAL_ATLAS_FIRST_ID)/MATERIAL_ATLAS_COLUMNS;
        bool varied=false;
        const std::size_t firstPixel=(static_cast<std::size_t>(ty*MATERIAL_ATLAS_TILE_SIZE)*
            MATERIAL_ATLAS_WIDTH+tx*MATERIAL_ATLAS_TILE_SIZE)*4u;
        for(unsigned y=0;y<MATERIAL_ATLAS_TILE_SIZE&&!varied;++y)
            for(unsigned x=0;x<MATERIAL_ATLAS_TILE_SIZE;++x) {
                const std::size_t at=firstPixel+
                    (static_cast<std::size_t>(y)*MATERIAL_ATLAS_WIDTH+x)*4u;
                if(first[at]!=first[firstPixel]||first[at+1]!=first[firstPixel+1]||
                   first[at+2]!=first[firstPixel+2]) { varied=true; break; }
            }
        assert(varied);
    }
    std::cout << "MINEPLUS procedural material atlas tests: OK\n";
    return 0;
}

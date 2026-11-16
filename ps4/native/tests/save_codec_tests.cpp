#include "save_codec.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace mineplus;
    WorldArchive source;
    source.seed=0xfedcba9876543210ULL;
    source.edits={{-33,48,9},{700,-900,21},{0,0,0}};
    std::vector<std::uint8_t> bytes;
    assert(SaveCodec::encode(source,bytes));
    WorldArchive restored;
    assert(SaveCodec::decode(bytes.data(),bytes.size(),restored));
    assert(restored.seed==source.seed && restored.edits.size()==3);
    for(std::size_t i=0;i<source.edits.size();++i) {
        assert(restored.edits[i].x==source.edits[i].x);
        assert(restored.edits[i].y==source.edits[i].y);
        assert(restored.edits[i].tile==source.edits[i].tile);
    }
    auto damaged=bytes; damaged[25]^=0x40;
    assert(!SaveCodec::decode(damaged.data(),damaged.size(),restored));
    assert(!SaveCodec::decode(bytes.data(),bytes.size()-1,restored));
    auto trailing=bytes; trailing.push_back(0);
    assert(!SaveCodec::decode(trailing.data(),trailing.size(),restored));
    WorldArchive duplicate; duplicate.edits={{1,2,3},{1,2,4}};
    assert(!SaveCodec::encode(duplicate,damaged));
    WorldArchive invalidTile; invalidTile.edits={{1,2,255}};
    assert(!SaveCodec::encode(invalidTile,damaged));
    std::cout << "MINEPLUS save codec tests: OK\n";
}

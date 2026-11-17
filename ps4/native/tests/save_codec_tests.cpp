#include "save_codec.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

static std::uint32_t test_checksum(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<size;++i) {
        crc^=data[i];
        for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
}
static void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));
}

int main() {
    using namespace mineplus;
    WorldArchive source;
    source.seed=0xfedcba9876543210ULL;
    source.edits={{-33,48,9},{700,-900,21},{0,0,0}};
    source.voxel_edits={{-33,48,12,VoxelMaterial::Crystal},{700,-900,42,VoxelMaterial::Air}};
    source.dungeon_chests={{77,false,{{DungeonLootItem::CrystalShard,4},{DungeonLootItem::BossSigil,1}}},
                           {99,true,{{DungeonLootItem::AncientRelic,2}}}};
    std::vector<std::uint8_t> bytes;
    assert(SaveCodec::encode(source,bytes));
    WorldArchive restored;
    assert(SaveCodec::decode(bytes.data(),bytes.size(),restored));
    assert(restored.seed==source.seed && restored.edits.size()==3);
    assert(restored.voxel_edits.size()==2);
    assert(restored.dungeon_chests.size()==2);
    assert(restored.dungeon_chests[0].room_id==77 && !restored.dungeon_chests[0].opened);
    assert(restored.dungeon_chests[0].contents.size()==2);
    assert(restored.dungeon_chests[0].contents[0].item==DungeonLootItem::CrystalShard);
    assert(restored.dungeon_chests[0].contents[0].count==4);
    assert(restored.dungeon_chests[1].opened && restored.dungeon_chests[1].contents[0].count==2);

    // Version 2 archives end after voxel edits; version 1 ends after surface edits.
    const std::size_t surfaceEnd=24+source.edits.size()*9;
    const std::size_t voxelEnd=surfaceEnd+4+source.voxel_edits.size()*13;
    std::vector<std::uint8_t> version2(bytes.begin(),bytes.begin()+voxelEnd);
    version2[8]=2; version2[9]=version2[10]=version2[11]=0;
    append_u32(version2,test_checksum(version2.data(),version2.size()));
    WorldArchive oldVersion2;
    assert(SaveCodec::decode(version2.data(),version2.size(),oldVersion2));
    assert(oldVersion2.voxel_edits.size()==2 && oldVersion2.dungeon_chests.empty());
    std::vector<std::uint8_t> version1(bytes.begin(),bytes.begin()+surfaceEnd);
    version1[8]=1; version1[9]=version1[10]=version1[11]=0;
    append_u32(version1,test_checksum(version1.data(),version1.size()));
    WorldArchive oldVersion1;
    assert(SaveCodec::decode(version1.data(),version1.size(),oldVersion1));
    assert(oldVersion1.voxel_edits.empty() && oldVersion1.dungeon_chests.empty());
    assert(restored.voxel_edits[0].x==-33 && restored.voxel_edits[0].z==12);
    assert(restored.voxel_edits[0].material==VoxelMaterial::Crystal);
    assert(restored.voxel_edits[1].material==VoxelMaterial::Air);
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
    WorldArchive duplicateVoxel; duplicateVoxel.voxel_edits={{1,2,3,VoxelMaterial::Stone},{1,2,3,VoxelMaterial::Air}};
    assert(!SaveCodec::encode(duplicateVoxel,damaged));
    WorldArchive invalidVoxel; invalidVoxel.voxel_edits={{1,2,128,VoxelMaterial::Stone}};
    assert(!SaveCodec::encode(invalidVoxel,damaged));
    WorldArchive duplicateChest; duplicateChest.dungeon_chests={{1,false,{}},{1,true,{}}};
    assert(!SaveCodec::encode(duplicateChest,damaged));
    WorldArchive invalidChest; invalidChest.dungeon_chests={{2,false,{{DungeonLootItem::BossSigil,0}}}};
    assert(!SaveCodec::encode(invalidChest,damaged));
    WorldArchive invalidItem; invalidItem.dungeon_chests={{3,false,{{static_cast<DungeonLootItem>(99),1}}}};
    assert(!SaveCodec::encode(invalidItem,damaged));
    WorldArchive extendedTile; extendedTile.edits={{1,2,255}};
    assert(SaveCodec::encode(extendedTile,damaged));
    WorldArchive extendedRestored;
    assert(SaveCodec::decode(damaged.data(),damaged.size(),extendedRestored));
    assert(extendedRestored.edits[0].tile==255);
    std::cout << "MINEPLUS save codec tests: OK\n";
}

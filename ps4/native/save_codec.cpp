#include "save_codec.hpp"
#include <algorithm>
#include <limits>

namespace mineplus {
namespace {
constexpr std::uint8_t MAGIC[8] = {'M','P','S','A','V','E','0','1'};
constexpr std::size_t HEADER_SIZE = 8 + 4 + 8 + 4;
constexpr std::size_t RECORD_SIZE = 4 + 4 + 1;
constexpr std::size_t VOXEL_RECORD_SIZE = 4 + 4 + 4 + 1;
constexpr std::size_t TRAILER_SIZE = 4;

void put32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (unsigned i=0;i<4;++i) out.push_back(static_cast<std::uint8_t>(v >> (i*8)));
}
void put64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (unsigned i=0;i<8;++i) out.push_back(static_cast<std::uint8_t>(v >> (i*8)));
}
std::uint32_t get32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) |
           (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);
}
std::uint64_t get64(const std::uint8_t* p) {
    std::uint64_t v=0; for(unsigned i=0;i<8;++i) v |= std::uint64_t(p[i])<<(i*8); return v;
}
std::uint32_t checksum(const std::uint8_t* p, std::size_t n) {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<n;++i) {
        crc ^= p[i];
        for(unsigned bit=0;bit<8;++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u-(crc & 1u)));
    }
    return ~crc;
}
bool valid_material(VoxelMaterial material) {
    const auto id=static_cast<std::uint8_t>(material);
    return id<=17 || id==22 || id==23;
}
bool valid_edits(const std::vector<ChunkCache::Edit>& edits) {
    if(edits.size()>SaveCodec::MAX_EDITS) return false;
    for(std::size_t i=0;i<edits.size();++i)
        for(std::size_t j=i+1;j<edits.size();++j)
            if(edits[i].x==edits[j].x && edits[i].y==edits[j].y) return false;
    return true;
}
bool valid_voxel_edits(const std::vector<ChunkCache::VoxelEdit>& edits) {
    if(edits.size()>SaveCodec::MAX_VOXEL_EDITS) return false;
    for(std::size_t i=0;i<edits.size();++i) {
        if(edits[i].z<0 || edits[i].z>=PcgWorld::WORLD_HEIGHT ||
           !valid_material(edits[i].material)) return false;
        for(std::size_t j=i+1;j<edits.size();++j)
            if(edits[i].x==edits[j].x && edits[i].y==edits[j].y &&
               edits[i].z==edits[j].z) return false;
    }
    return true;
}
}

bool SaveCodec::encode(const WorldArchive& archive, std::vector<std::uint8_t>& bytes) {
    if(!valid_edits(archive.edits) || !valid_voxel_edits(archive.voxel_edits)) return false;
    std::vector<std::uint8_t> result;
    result.reserve(HEADER_SIZE + archive.edits.size()*RECORD_SIZE + 4 +
                   archive.voxel_edits.size()*VOXEL_RECORD_SIZE + TRAILER_SIZE);
    result.insert(result.end(), MAGIC, MAGIC+8);
    put32(result, VERSION);
    put64(result, archive.seed);
    put32(result, static_cast<std::uint32_t>(archive.edits.size()));
    for(const auto& e:archive.edits) {
        put32(result, static_cast<std::uint32_t>(e.x));
        put32(result, static_cast<std::uint32_t>(e.y));
        result.push_back(e.tile);
    }
    put32(result, static_cast<std::uint32_t>(archive.voxel_edits.size()));
    for(const auto& e:archive.voxel_edits) {
        put32(result, static_cast<std::uint32_t>(e.x));
        put32(result, static_cast<std::uint32_t>(e.y));
        put32(result, static_cast<std::uint32_t>(e.z));
        result.push_back(static_cast<std::uint8_t>(e.material));
    }
    put32(result, checksum(result.data(), result.size()));
    bytes.swap(result);
    return true;
}

bool SaveCodec::decode(const std::uint8_t* bytes, std::size_t size, WorldArchive& archive) {
    if(!bytes || size<HEADER_SIZE+TRAILER_SIZE) return false;
    if(!std::equal(MAGIC,MAGIC+8,bytes)) return false;
    const std::uint32_t version=get32(bytes+8);
    if(version!=1 && version!=VERSION) return false;
    const std::uint32_t count=get32(bytes+20);
    if(count>MAX_EDITS) return false;
    const std::size_t surface_end=HEADER_SIZE+std::size_t(count)*RECORD_SIZE;
    if(surface_end>size-TRAILER_SIZE) return false;
    std::uint32_t voxel_count=0;
    std::size_t expected=surface_end+TRAILER_SIZE;
    if(version>=2) {
        if(surface_end+4>size-TRAILER_SIZE) return false;
        voxel_count=get32(bytes+surface_end);
        if(voxel_count>MAX_VOXEL_EDITS) return false;
        expected=surface_end+4+std::size_t(voxel_count)*VOXEL_RECORD_SIZE+TRAILER_SIZE;
    }
    if(size!=expected) return false;
    if(get32(bytes+size-TRAILER_SIZE)!=checksum(bytes,size-TRAILER_SIZE)) return false;

    WorldArchive candidate;
    candidate.seed=get64(bytes+12);
    candidate.edits.reserve(count);
    std::size_t pos=HEADER_SIZE;
    for(std::uint32_t i=0;i<count;++i) {
        const auto x=static_cast<std::int32_t>(get32(bytes+pos)); pos+=4;
        const auto y=static_cast<std::int32_t>(get32(bytes+pos)); pos+=4;
        const auto tile=bytes[pos++];
        candidate.edits.push_back(ChunkCache::Edit{x,y,tile});
    }
    if(version>=2) {
        pos+=4;
        candidate.voxel_edits.reserve(voxel_count);
        for(std::uint32_t i=0;i<voxel_count;++i) {
            const auto x=static_cast<std::int32_t>(get32(bytes+pos)); pos+=4;
            const auto y=static_cast<std::int32_t>(get32(bytes+pos)); pos+=4;
            const auto z=static_cast<std::int32_t>(get32(bytes+pos)); pos+=4;
            const auto material=static_cast<VoxelMaterial>(bytes[pos++]);
            candidate.voxel_edits.push_back(ChunkCache::VoxelEdit{x,y,z,material});
        }
    }
    if(!valid_edits(candidate.edits) || !valid_voxel_edits(candidate.voxel_edits)) return false;
    archive.seed=candidate.seed;
    archive.edits.swap(candidate.edits);
    archive.voxel_edits.swap(candidate.voxel_edits);
    return true;
}

} // namespace mineplus

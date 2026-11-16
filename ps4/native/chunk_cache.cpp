#include "chunk_cache.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstddef>

namespace mineplus {

ChunkCache::ChunkCache(std::uint64_t seed)
    : generator_(seed), generator_seed_(seed) {}

std::size_t ChunkCache::local_index(std::int32_t x, std::int32_t y) {
    constexpr std::int32_t S = PcgWorld::CHUNK_SIZE;
    const std::int32_t lx = x - ChunkGenerator::floor_div(x, S) * S;
    const std::int32_t ly = y - ChunkGenerator::floor_div(y, S) * S;
    return static_cast<std::size_t>(ly * S + lx);
}

ChunkCache::Entry* ChunkCache::find_chunk(std::int32_t cx, std::int32_t cy) {
    for (auto& entry : entries_) {
        if (entry.valid && entry.data.chunk_x == cx && entry.data.chunk_y == cy) {
            entry.stamp = clock_++;
            return &entry;
        }
    }
    return nullptr;
}

void ChunkCache::apply_deltas(ChunkData& chunk) {
    for (const auto& delta : deltas_) {
        if (!delta.valid) continue;
        if (ChunkGenerator::floor_div(delta.x, PcgWorld::CHUNK_SIZE) != chunk.chunk_x ||
            ChunkGenerator::floor_div(delta.y, PcgWorld::CHUNK_SIZE) != chunk.chunk_y)
            continue;
        const auto index = local_index(delta.x, delta.y);
        chunk.cells[index].surface = delta.tile;
    }
}

ChunkCache::Entry* ChunkCache::acquire_chunk(std::int32_t cx, std::int32_t cy) {
    if (auto* found = find_chunk(cx, cy)) return found;

    Entry* victim = nullptr;
    for (auto& entry : entries_) {
        if (!entry.valid) { victim = &entry; break; }
        if (!victim || entry.stamp < victim->stamp) victim = &entry;
    }
    if (!victim) return nullptr;

    generator_.generate(cx, cy, victim->data);
    apply_deltas(victim->data);
    victim->valid = true;
    victim->stamp = clock_++;
    return victim;
}

void ChunkCache::update_center(std::int32_t wx, std::int32_t wy,
                               std::int32_t radius) {
    radius = std::clamp(radius, 0, 4);
    const auto cx = ChunkGenerator::floor_div(wx, PcgWorld::CHUNK_SIZE);
    const auto cy = ChunkGenerator::floor_div(wy, PcgWorld::CHUNK_SIZE);

    // Load nearest first; with radius 4 this exactly fills the 9x9 cache.
    for (std::int32_t ring = 0; ring <= radius; ++ring) {
        for (std::int32_t oy = -ring; oy <= ring; ++oy) {
            for (std::int32_t ox = -ring; ox <= ring; ++ox) {
                if (std::max(std::abs(ox), std::abs(oy)) != ring) continue;
                const std::int64_t tx = std::int64_t(cx) + ox;
                const std::int64_t ty = std::int64_t(cy) + oy;
                if (tx < INT32_MIN || tx > INT32_MAX ||
                    ty < INT32_MIN || ty > INT32_MAX) continue;
                acquire_chunk(static_cast<std::int32_t>(tx),
                              static_cast<std::int32_t>(ty));
            }
        }
    }
}

bool ChunkCache::get(std::int32_t wx, std::int32_t wy, SurfaceSample& out) {
    const auto cx = ChunkGenerator::floor_div(wx, PcgWorld::CHUNK_SIZE);
    const auto cy = ChunkGenerator::floor_div(wy, PcgWorld::CHUNK_SIZE);
    auto* entry = acquire_chunk(cx, cy);
    if (!entry) return false;
    out = entry->data.cells[local_index(wx, wy)];
    return true;
}

bool ChunkCache::get_voxel(std::int32_t wx, std::int32_t wy,
                            std::int32_t z, VoxelSample& out) {
    const auto cx = ChunkGenerator::floor_div(wx, PcgWorld::CHUNK_SIZE);
    const auto cy = ChunkGenerator::floor_div(wy, PcgWorld::CHUNK_SIZE);
    auto* entry = acquire_chunk(cx, cy);
    if (!entry) return false;
    const SurfaceSample& column = entry->data.cells[local_index(wx, wy)];
    // The editable surface is authoritative at its height: mining and placing
    // must be visible through both the 2D compatibility API and 3D voxel API.
    if (z == column.height) {
        const std::uint8_t id = column.surface;
        // Surface tile IDs are sparse: keep the extended volcanic/ore IDs
        // while rejecting gaps so an arbitrary byte never becomes an invalid enum.
        VoxelMaterial material = VoxelMaterial::Stone;
        switch (id) {
            case 0: material = VoxelMaterial::Air; break;
            case 1: material = VoxelMaterial::Grass; break;
            case 2: material = VoxelMaterial::Dirt; break;
            case 3: material = VoxelMaterial::Stone; break;
            case 4: material = VoxelMaterial::Wood; break;
            case 5: material = VoxelMaterial::Moss; break;
            case 6: material = VoxelMaterial::Crystal; break;
            case 7: material = VoxelMaterial::Bone; break;
            case 8: material = VoxelMaterial::DarkStone; break;
            case 9: material = VoxelMaterial::Sand; break;
            case 10: material = VoxelMaterial::Snow; break;
            case 11: material = VoxelMaterial::Ice; break;
            case 12: material = VoxelMaterial::Lava; break;
            case 13: material = VoxelMaterial::Water; break;
            case 14: material = VoxelMaterial::IronOre; break;
            case 15: material = VoxelMaterial::Obsidian; break;
            case 16: material = VoxelMaterial::Ember; break;
            case 17: material = VoxelMaterial::Rune; break;
            case 22: material = VoxelMaterial::VolcanicRock; break;
            case 23: material = VoxelMaterial::Ore; break;
            default: material = VoxelMaterial::Stone; break;
        }
        const bool liquid = material == VoxelMaterial::Water ||
                            material == VoxelMaterial::Lava;
        out = VoxelSample{material, material != VoxelMaterial::Air && !liquid, liquid};
        return true;
    }
    out = generator_.sample_voxel(wx, wy, z);
    return true;
}

bool ChunkCache::get_structure(std::int32_t wx, std::int32_t wy,
                                 StructureAnchor& out) {
    const auto cx = ChunkGenerator::floor_div(wx, PcgWorld::CHUNK_SIZE);
    const auto cy = ChunkGenerator::floor_div(wy, PcgWorld::CHUNK_SIZE);
    auto* entry = acquire_chunk(cx, cy);
    if (!entry) return false;
    const auto index = local_index(wx, wy);
    if (!entry->data.has_structure[index]) return false;
    out = entry->data.structures[index];
    return true;
}

bool ChunkCache::set_surface(std::int32_t wx, std::int32_t wy,
                             std::uint8_t tile) {
    const auto cx = ChunkGenerator::floor_div(wx, PcgWorld::CHUNK_SIZE);
    const auto cy = ChunkGenerator::floor_div(wy, PcgWorld::CHUNK_SIZE);
    auto* entry = acquire_chunk(cx, cy);
    if (!entry) return false;

    Delta* slot = nullptr;
    for (auto& delta : deltas_) {
        if (delta.valid && delta.x == wx && delta.y == wy) {
            slot = &delta;
            break;
        }
        if (!delta.valid && !slot) slot = &delta;
    }
    if (!slot) return false; // Never silently discard a player's world edit.

    slot->x = wx;
    slot->y = wy;
    slot->tile = tile;
    slot->valid = true;
    entry->data.cells[local_index(wx, wy)].surface = tile;
    return true;
}

std::size_t ChunkCache::export_edits(Edit* output, std::size_t capacity) const {
    const std::size_t required = delta_count();
    if (required == 0) return 0;
    if (!output || capacity < required) return 0;
    std::size_t written = 0;
    for (const auto& delta : deltas_) {
        if (!delta.valid) continue;
        output[written++] = Edit{delta.x, delta.y, delta.tile};
    }
    return written;
}

bool ChunkCache::import_edits(const Edit* edits, std::size_t count) {
    if ((count != 0 && !edits) || count > DELTA_CAPACITY) return false;
    // Validate the complete input before mutating the live journal.
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1; j < count; ++j) {
            if (edits[i].x == edits[j].x && edits[i].y == edits[j].y)
                return false;
        }
    }
    for (auto& delta : deltas_) delta = Delta{};
    for (std::size_t i = 0; i < count; ++i) {
        deltas_[i] = Delta{edits[i].x, edits[i].y, edits[i].tile, true};
        const auto cx = ChunkGenerator::floor_div(edits[i].x, PcgWorld::CHUNK_SIZE);
        const auto cy = ChunkGenerator::floor_div(edits[i].y, PcgWorld::CHUNK_SIZE);
        if (auto* entry = find_chunk(cx, cy)) {
            entry->data.cells[local_index(edits[i].x, edits[i].y)].surface = edits[i].tile;
        }
    }
    return true;
}

bool ChunkCache::restore(std::uint64_t seed, const Edit* edits, std::size_t count) {
    // Validate before resetting any live state. No large temporary cache is
    // placed on the PS4 thread stack during load.
    if ((count != 0 && !edits) || count > DELTA_CAPACITY) return false;
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1; j < count; ++j) {
            if (edits[i].x == edits[j].x && edits[i].y == edits[j].y)
                return false;
        }
    }
    generator_ = ChunkGenerator(seed);
    generator_seed_ = seed;
    entries_ = {};
    deltas_ = {};
    clock_ = 1;
    return import_edits(edits, count);
}

std::size_t ChunkCache::loaded_count() const {
    std::size_t count = 0;
    for (const auto& entry : entries_) if (entry.valid) ++count;
    return count;
}

std::size_t ChunkCache::delta_count() const {
    std::size_t count = 0;
    for (const auto& delta : deltas_) if (delta.valid) ++count;
    return count;
}

} // namespace mineplus

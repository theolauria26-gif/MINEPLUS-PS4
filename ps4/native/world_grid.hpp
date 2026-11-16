#pragma once
#include "world_runtime.hpp"
#include <cstddef>
#include <cstdint>

namespace mineplus {

// Shared contract lets gameplay switch terrain storage without changing
// collision, mining, placement, or rendering call sites.
class WorldGridView {
public:
    virtual ~WorldGridView() = default;
    virtual bool contains(std::int32_t x, std::int32_t y) const = 0;
    virtual bool get(std::int32_t x, std::int32_t y, std::uint8_t& tile) = 0;
    virtual bool set(std::int32_t x, std::int32_t y, std::uint8_t tile) = 0;
};

// Transitional adapter for the resident legacy map.
class FixedWorldGrid final : public WorldGridView {
public:
    FixedWorldGrid(std::uint8_t* cells, std::int32_t width, std::int32_t height)
        : cells_(cells), width_(width), height_(height) {}

    bool contains(std::int32_t x, std::int32_t y) const override {
        return cells_ && x >= 0 && y >= 0 && x < width_ && y < height_;
    }

    bool get(std::int32_t x, std::int32_t y, std::uint8_t& tile) override {
        if (!contains(x, y)) return false;
        tile = cells_[static_cast<std::size_t>(y) *
                      static_cast<std::size_t>(width_) +
                      static_cast<std::size_t>(x)];
        return true;
    }

    bool set(std::int32_t x, std::int32_t y, std::uint8_t tile) override {
        if (!contains(x, y)) return false;
        cells_[static_cast<std::size_t>(y) *
               static_cast<std::size_t>(width_) +
               static_cast<std::size_t>(x)] = tile;
        return true;
    }

    std::int32_t width() const { return width_; }
    std::int32_t height() const { return height_; }

private:
    std::uint8_t* cells_;
    std::int32_t width_;
    std::int32_t height_;
};

// Signed world coordinates are unlimited by the resident map dimensions.
// Reads acquire procedural chunks; writes are retained in the edit journal.
class StreamedWorldGrid final : public WorldGridView {
public:
    explicit StreamedWorldGrid(WorldRuntime& runtime) : runtime_(runtime) {}
    bool contains(std::int32_t, std::int32_t) const override { return true; }
    bool get(std::int32_t x, std::int32_t y, std::uint8_t& tile) override {
        return runtime_.get_tile(x, y, tile);
    }
    bool set(std::int32_t x, std::int32_t y, std::uint8_t tile) override {
        return runtime_.set_tile(x, y, tile);
    }
    bool get_sample(std::int32_t x, std::int32_t y, SurfaceSample& sample) {
        return runtime_.get_sample(x, y, sample);
    }
    bool get_structure(std::int32_t x, std::int32_t y, StructureAnchor& anchor) {
        return runtime_.get_structure(x, y, anchor);
    }
private:
    WorldRuntime& runtime_;
};

} // namespace mineplus

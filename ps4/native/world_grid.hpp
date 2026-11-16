#pragma once
#include <cstddef>
#include <cstdint>
#include "world_runtime.hpp"

namespace mineplus {\n\n// Common gameplay contract for resident and streamed terrain backends.\nclass WorldGridView {\npublic:\n    virtual ~WorldGridView() = default;\n    virtual bool contains(std::int32_t x, std::int32_t y) const = 0;\n    virtual bool get(std::int32_t x, std::int32_t y, std::uint8_t& tile) = 0;\n    virtual bool set(std::int32_t x, std::int32_t y, std::uint8_t tile) = 0;\n};

// Transitional adapter for the resident legacy map. It centralizes bounds
// checks and tile mutation so gameplay can migrate to streamed coordinates
// without touching save layout or procedural-generation code in one step.
class FixedWorldGrid : public WorldGridView {
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

} // namespace mineplus

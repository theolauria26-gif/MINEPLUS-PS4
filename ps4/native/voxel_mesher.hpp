#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace mineplus {

// A 16x16x128 voxel chunk plus a one-voxel halo on every side.
// Coordinates accepted by set()/get() are local chunk coordinates; halo
// coordinates (-1 and SIZE) let adjacent chunks hide shared boundary faces.
struct VoxelChunk {
    static constexpr int SIZE_X = 16;
    static constexpr int SIZE_Y = 16;
    static constexpr int SIZE_Z = 128;
    static constexpr int PAD_X = SIZE_X + 2;
    static constexpr int PAD_Y = SIZE_Y + 2;
    static constexpr int PAD_Z = SIZE_Z + 2;
    static constexpr std::size_t CELL_COUNT =
        static_cast<std::size_t>(PAD_X) * PAD_Y * PAD_Z;

    std::array<std::uint8_t, CELL_COUNT> cells{};

    bool set(int x, int y, int z, std::uint8_t material);
    std::uint8_t get(int x, int y, int z) const;
private:
    static std::size_t index(int x, int y, int z);
};

enum class VoxelFace : std::uint8_t {
    NegativeX, PositiveX, NegativeY, PositiveY, NegativeZ, PositiveZ
};

// One greedy-merged rectangle. The face determines its normal; width/height
// span the two remaining axes. Positions are chunk-local voxel-grid units.
struct MeshQuad {
    std::uint8_t x = 0, y = 0, z = 0;
    std::uint8_t width = 0, height = 0;
    std::uint8_t material = 0;
    VoxelFace face = VoxelFace::PositiveZ;
};

class VoxelMesher {
public:
    // Clears output and emits only faces adjacent to air. Equal-material
    // coplanar faces are merged into rectangles to reduce draw-call geometry.
    // Returns false and clears output if max_quads would be exceeded, allowing
    // callers to enforce a predictable per-chunk memory budget on PS4.
    static bool build(const VoxelChunk& chunk, std::vector<MeshQuad>& output,
                      std::size_t max_quads = 65536);
};

} // namespace mineplus

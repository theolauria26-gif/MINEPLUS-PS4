#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace mineplus {
constexpr std::size_t SURFACE_QUEST_ROLES = 5;
enum class SurfaceQuestState : std::uint8_t { NotAccepted=0, Active=1, Claimed=2 };
enum class SurfaceQuestAction : std::uint8_t { Accepted, Progress, ReadyToClaim, Claimed, AlreadyClaimed, Invalid };
struct SurfaceQuestDefinition {
    std::uint8_t item_id;
    std::uint16_t required;
    std::uint8_t reward_item_id;
    std::uint16_t reward_amount;
    const char* title;
    const char* description;
};
struct SurfaceQuestProgress {
    std::uint8_t state;
    std::uint16_t gathered;
};
SurfaceQuestDefinition surface_quest_definition(std::uint8_t role);
void surface_quest_add_progress(std::array<SurfaceQuestProgress, SURFACE_QUEST_ROLES>& progress,
                                std::uint8_t item_id, std::uint16_t amount);
SurfaceQuestAction surface_quest_interact(std::uint8_t role,
    std::array<SurfaceQuestProgress, SURFACE_QUEST_ROLES>& progress,
    int* inventory, std::size_t inventory_count);
bool surface_quest_progress_valid(const std::array<SurfaceQuestProgress, SURFACE_QUEST_ROLES>& progress);
} // namespace mineplus

#include "surface_quests.hpp"
#include <algorithm>

namespace mineplus {
namespace {
constexpr SurfaceQuestDefinition quests[SURFACE_QUEST_ROLES] = {
    {27,3,22,18,"ELDER'S MEMORY","Bring 3 Rune Fragments to restore the village chronicle"},
    {18,3,22,24,"SMITH'S SUPPLY","Gather 3 Iron Bars for the frontier forge"},
    {23,4,22,16,"HERBALIST'S REMEDY","Collect 4 Herbs for a healing draught"},
    {26,3,22,20,"SCOUT'S SIGNAL","Find 3 Crystal Shards to mark the old road"},
    {24,4,30,1,"WARDEN'S OATH","Gather 4 Bone Shards to reinforce the watch"}
};
}
SurfaceQuestDefinition surface_quest_definition(std::uint8_t role) {
    return role < SURFACE_QUEST_ROLES ? quests[role] : SurfaceQuestDefinition{};
}
void surface_quest_add_progress(std::array<SurfaceQuestProgress, SURFACE_QUEST_ROLES>& progress,
                                std::uint8_t item_id, std::uint16_t amount) {
    if (!amount) return;
    for (std::size_t i=0;i<progress.size();++i) {
        auto& q=progress[i];
        if (q.state != static_cast<std::uint8_t>(SurfaceQuestState::Active) ||
            quests[i].item_id != item_id) continue;
        q.gathered=static_cast<std::uint16_t>(std::min<unsigned>(
            quests[i].required, static_cast<unsigned>(q.gathered)+amount));
    }
}
SurfaceQuestAction surface_quest_interact(std::uint8_t role,
    std::array<SurfaceQuestProgress, SURFACE_QUEST_ROLES>& progress,
    int* inventory, std::size_t inventory_count) {
    if (role>=SURFACE_QUEST_ROLES || !inventory ||
        inventory_count<=quests[role].item_id ||
        inventory_count<=quests[role].reward_item_id) return SurfaceQuestAction::Invalid;
    auto& q=progress[role];
    if (q.state==static_cast<std::uint8_t>(SurfaceQuestState::Claimed))
        return SurfaceQuestAction::AlreadyClaimed;
    if (q.state==static_cast<std::uint8_t>(SurfaceQuestState::NotAccepted)) {
        q.state=static_cast<std::uint8_t>(SurfaceQuestState::Active);
        q.gathered=0;
        return SurfaceQuestAction::Accepted;
    }
    if (q.state!=static_cast<std::uint8_t>(SurfaceQuestState::Active) ||
        q.gathered>quests[role].required) return SurfaceQuestAction::Invalid;
    if (q.gathered<quests[role].required) return SurfaceQuestAction::Progress;
    const auto& offer=quests[role];
    if (inventory[offer.reward_item_id]<0 ||
        inventory[offer.reward_item_id]>9999-static_cast<int>(offer.reward_amount))
        return SurfaceQuestAction::Invalid;
    inventory[offer.reward_item_id]+=offer.reward_amount;
    q.state=static_cast<std::uint8_t>(SurfaceQuestState::Claimed);
    return SurfaceQuestAction::Claimed;
}
bool surface_quest_progress_valid(const std::array<SurfaceQuestProgress, SURFACE_QUEST_ROLES>& progress) {
    for(std::size_t i=0;i<progress.size();++i) {
        const auto& q=progress[i];
        if(q.state>static_cast<std::uint8_t>(SurfaceQuestState::Claimed) ||
           q.gathered>quests[i].required ||
           (q.state==static_cast<std::uint8_t>(SurfaceQuestState::NotAccepted)&&q.gathered!=0) ||
           (q.state==static_cast<std::uint8_t>(SurfaceQuestState::Claimed)&&q.gathered!=quests[i].required))
            return false;
    }
    return true;
}
} // namespace mineplus

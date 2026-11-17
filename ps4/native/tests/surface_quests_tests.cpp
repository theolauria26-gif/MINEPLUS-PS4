#include "surface_quests.hpp"
#include <array>
#include <cassert>
#include <cstdio>
int main() {
    using namespace mineplus;
    std::array<SurfaceQuestProgress,SURFACE_QUEST_ROLES> progress{};
    std::array<int,31> inventory{};
    for(std::uint8_t role=0;role<SURFACE_QUEST_ROLES;++role) {
        const auto def=surface_quest_definition(role);
        assert(def.required>0 && def.reward_amount>0 && def.title && def.description);
        assert(surface_quest_interact(role,progress,inventory.data(),inventory.size())==SurfaceQuestAction::Accepted);
        surface_quest_add_progress(progress,def.item_id,static_cast<std::uint16_t>(def.required-1));
        assert(surface_quest_interact(role,progress,inventory.data(),inventory.size())==SurfaceQuestAction::Progress);
        surface_quest_add_progress(progress,def.item_id,1);
        assert(surface_quest_interact(role,progress,inventory.data(),inventory.size())==SurfaceQuestAction::Claimed);
        assert(inventory[def.reward_item_id]==def.reward_amount);
        assert(surface_quest_interact(role,progress,inventory.data(),inventory.size())==SurfaceQuestAction::AlreadyClaimed);
    }
    assert(surface_quest_progress_valid(progress));
    auto invalid=progress; invalid[0].gathered=999;
    assert(!surface_quest_progress_valid(invalid));
    auto full=std::array<SurfaceQuestProgress,SURFACE_QUEST_ROLES>{};
    const auto d=surface_quest_definition(0);
    assert(surface_quest_interact(0,full,inventory.data(),inventory.size())==SurfaceQuestAction::Accepted);
    surface_quest_add_progress(full,d.item_id,d.required);
    inventory[d.reward_item_id]=9999;
    assert(surface_quest_interact(0,full,inventory.data(),inventory.size())==SurfaceQuestAction::Invalid);
    assert(full[0].state==static_cast<std::uint8_t>(SurfaceQuestState::Active));
    assert(surface_quest_interact(99,full,inventory.data(),inventory.size())==SurfaceQuestAction::Invalid);
    assert(surface_quest_interact(0,full,nullptr,inventory.size())==SurfaceQuestAction::Invalid);
    std::puts("Surface quest tests passed");
}

#pragma once
#include <cstdint>
namespace mineplus {
struct SettlementNpc { std::int32_t x,y,home_x,home_y; std::uint32_t seed; std::uint8_t role,dialogue_stage; };
struct NpcDialogue { const char* name; const char* line; };
enum class SettlementNpcRole : std::uint8_t { Elder=0,Smith=1,Herbalist=2,Scout=3,Warden=4 };
void settlement_npc_goal(const SettlementNpc&,std::uint32_t,std::int32_t&,std::int32_t&);
NpcDialogue settlement_npc_dialogue(std::uint8_t,std::uint8_t);
} // namespace mineplus

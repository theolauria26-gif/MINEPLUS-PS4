#include "surface_npcs.hpp"
namespace mineplus {
namespace {
constexpr std::int8_t patrol[5][5][2]={
 {{0,0},{-1,0},{0,-1},{1,0},{0,0}},{{1,0},{2,0},{1,1},{0,1},{0,0}},
 {{-1,0},{-2,0},{-1,1},{0,1},{0,0}},{{0,-1},{0,-2},{1,-1},{1,0},{0,0}},
 {{0,1},{0,2},{-1,1},{-1,0},{0,0}}
};
constexpr NpcDialogue lines[5][3]={
 {{"ELDER","THE OLD ROAD STILL REMEMBERS"},{"ELDER","KEEP THE HEARTH LIT AFTER DUSK"},{"ELDER","RUINS TO THE EAST HOLD OLD SECRETS"}},
 {{"SMITH","IRON AND PATIENCE MAKE GOOD ARMOR"},{"SMITH","BRING ORE AND I CAN FORGE STRONGER GEAR"},{"SMITH","WATCH YOUR BLADE DURABILITY OUT THERE"}},
 {{"HERBALIST","THE FOREST GIVES TO THOSE WHO LISTEN"},{"HERBALIST","FOLLOW THE RIVER FOR RARE HERBS"},{"HERBALIST","PREDATORS HUNT NEAR THE DEEP GROVES"}},
 {{"SCOUT","I MARKED A TRACK BEYOND THE CLEARING"},{"SCOUT","OLD WATCH POSTS CAN HIDE TREASURE"},{"SCOUT","STAY OFF THE BLACK STONE WHEN IT GLOWS"}},
 {{"WARDEN","WE KEEP THIS PLACE SAFE TOGETHER"},{"WARDEN","THE WILDS CHANGE BUT OUR OATH DOES NOT"},{"WARDEN","IF THE HEARTH FALLS DARK WE MOVE ON"}}
};
}
void settlement_npc_goal(const SettlementNpc& n,std::uint32_t tick,std::int32_t& x,std::int32_t& y){
 const std::uint8_t role=static_cast<std::uint8_t>(n.role%5u);
 const std::uint32_t phase=(tick/10u+n.seed%5u)%5u;
 x=n.home_x+patrol[role][phase][0];y=n.home_y+patrol[role][phase][1];
}
NpcDialogue settlement_npc_dialogue(std::uint8_t role,std::uint8_t stage){return lines[role%5u][stage%3u];}
} // namespace mineplus

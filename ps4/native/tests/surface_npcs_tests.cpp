#include "surface_npcs.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
int main(){
 using namespace mineplus;
 for(std::uint8_t role=0;role<5u;++role){
  SettlementNpc npc{100,200,100,200,0x12345678u,role,0};
  for(std::uint32_t tick=0;tick<100u;++tick){std::int32_t x=0,y=0;settlement_npc_goal(npc,tick,x,y);assert(x>=98&&x<=102&&y>=198&&y<=202);}
  const auto a=settlement_npc_dialogue(role,0),b=settlement_npc_dialogue(role,1),c=settlement_npc_dialogue(role,2);
  assert(a.name&&a.line&&b.name&&b.line&&c.name&&c.line);
  assert(a.name[0]!='\0'&&a.line[0]!='\0');assert(a.line!=b.line&&b.line!=c.line);
  assert(settlement_npc_dialogue(role,3).line==a.line);
 }
 assert(settlement_npc_dialogue(255u,255u).name!=nullptr);
 std::cout<<"MINEPLUS surface NPC tests: OK\n";return 0;
}

#include "surface_events.hpp"
#include <cassert>
#include <cstdio>
int main() {
 using namespace mineplus;
 const auto first=surface_event_at(12345u,0);
 assert(first.type!=SurfaceEventType::None);
 assert(surface_event_at(12345u,1000).type==first.type);
 assert(surface_event_at(12345u,SURFACE_EVENT_ACTIVE_MS-1).type==first.type);
 assert(surface_event_at(12345u,SURFACE_EVENT_ACTIVE_MS).type==SurfaceEventType::None);
 assert(surface_event_at(12345u,SURFACE_EVENT_CYCLE_MS).type!=SurfaceEventType::None);
 assert(surface_event_at(12345u,SURFACE_EVENT_CYCLE_MS*2u+SURFACE_EVENT_ACTIVE_MS).type==SurfaceEventType::None);
 assert(surface_event_at(12345u,0).type==surface_event_at(12345u,0).type);
 assert(surface_event_trade_discount(SurfaceEventType::CaravanMarket)==25u);
 assert(surface_event_trade_discount(SurfaceEventType::HarvestFestival)==0u);
 assert(surface_event_gather_bonus(SurfaceEventType::HarvestFestival,23)==1u);
 assert(surface_event_gather_bonus(SurfaceEventType::HarvestFestival,22)==0u);
 assert(surface_event_gather_bonus(SurfaceEventType::RuneStorm,17)==1u);
 assert(surface_event_gather_bonus(SurfaceEventType::RuneStorm,27)==1u);
 assert(surface_event_gather_bonus(SurfaceEventType::RuneStorm,24)==0u);
 assert(surface_event_creature_interval(SurfaceEventType::WildlifeMigration)<650u);
 assert(surface_event_creature_interval(SurfaceEventType::PredatorWarning)<650u);
 assert(surface_event_creature_interval(SurfaceEventType::None)==650u);
 std::puts("Surface event tests passed");
}

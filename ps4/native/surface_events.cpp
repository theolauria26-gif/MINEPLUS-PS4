#include "surface_events.hpp"
namespace mineplus {
namespace {
std::uint32_t mix(std::uint32_t value) {
    value^=value>>16; value*=0x7feb352du; value^=value>>15;
    value*=0x846ca68bu; value^=value>>16; return value;
}
constexpr SurfaceEventInfo events[5]={
    {SurfaceEventType::CaravanMarket,"CARAVAN MARKET","TRADING PRICES REDUCED BY 25 PERCENT"},
    {SurfaceEventType::HarvestFestival,"HARVEST FESTIVAL","GATHER EXTRA FRONTIER SUPPLIES"},
    {SurfaceEventType::WildlifeMigration,"WILDLIFE MIGRATION","WILDLIFE IS ROAMING MORE ACTIVELY"},
    {SurfaceEventType::RuneStorm,"RUNE STORM","CRYSTALS AND RUNIC MATERIALS ARE ABUNDANT"},
    {SurfaceEventType::PredatorWarning,"PREDATOR WARNING","DANGEROUS CREATURES ARE MOVING FASTER"}
};
}
SurfaceEventInfo surface_event_at(std::uint32_t world_seed,std::uint64_t elapsed_ms) {
    const auto cycle=elapsed_ms/SURFACE_EVENT_CYCLE_MS;
    const auto phase=static_cast<std::uint32_t>(elapsed_ms%SURFACE_EVENT_CYCLE_MS);
    if(phase>=SURFACE_EVENT_ACTIVE_MS)return {SurfaceEventType::None,"",""};
    const auto index=mix(world_seed^static_cast<std::uint32_t>(cycle*0x9e3779b9u))%5u;
    return events[index];
}
std::uint8_t surface_event_trade_discount(SurfaceEventType type) {
    return type==SurfaceEventType::CaravanMarket?25u:0u;
}
std::uint8_t surface_event_gather_bonus(SurfaceEventType type,std::uint8_t item_id) {
    if(type==SurfaceEventType::HarvestFestival&&item_id>=23u&&item_id<=29u)return 1u;
    if(type==SurfaceEventType::RuneStorm&&(item_id==17u||item_id==26u||item_id==27u))return 1u;
    return 0u;
}
std::uint32_t surface_event_creature_interval(SurfaceEventType type) {
    if(type==SurfaceEventType::WildlifeMigration)return 420u;
    if(type==SurfaceEventType::PredatorWarning)return 460u;
    return 650u;
}
std::uint32_t surface_event_time_remaining(std::uint64_t elapsed_ms) {
    const auto phase=static_cast<std::uint32_t>(elapsed_ms%SURFACE_EVENT_CYCLE_MS);
    return phase<SURFACE_EVENT_ACTIVE_MS?SURFACE_EVENT_ACTIVE_MS-phase:SURFACE_EVENT_CYCLE_MS-phase;
}
std::uint8_t surface_event_alert_distance(SurfaceEventType type,bool passive) {
    if(type==SurfaceEventType::WildlifeMigration&&passive)return 11u;
    if(type==SurfaceEventType::PredatorWarning&&!passive)return 20u;
    return passive?7u:14u;
}
} // namespace mineplus

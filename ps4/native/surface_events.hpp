#pragma once
#include <cstdint>
namespace mineplus {
enum class SurfaceEventType : std::uint8_t {
    None=0, CaravanMarket=1, HarvestFestival=2, WildlifeMigration=3,
    RuneStorm=4, PredatorWarning=5
};
struct SurfaceEventInfo {
    SurfaceEventType type;
    const char* title;
    const char* description;
};
constexpr std::uint32_t SURFACE_EVENT_CYCLE_MS=180000u;
constexpr std::uint32_t SURFACE_EVENT_ACTIVE_MS=60000u;
SurfaceEventInfo surface_event_at(std::uint32_t world_seed,std::uint64_t elapsed_ms);
std::uint8_t surface_event_trade_discount(SurfaceEventType type);
std::uint8_t surface_event_gather_bonus(SurfaceEventType type,std::uint8_t item_id);
std::uint32_t surface_event_creature_interval(SurfaceEventType type);
} // namespace mineplus

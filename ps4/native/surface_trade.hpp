#pragma once
#include <cstddef>
#include <cstdint>
namespace mineplus {
struct SurfaceTradeOffer { std::uint8_t item_id; std::uint16_t coin_cost; const char* name; };
constexpr std::size_t SURFACE_TRADE_OFFERS=3;
SurfaceTradeOffer surface_trade_offer(std::uint8_t role,std::uint8_t offer);
bool surface_trade_buy(std::uint8_t role,std::uint8_t offer,int* inventory,std::size_t count);
}

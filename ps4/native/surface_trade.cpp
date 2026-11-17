#include "surface_trade.hpp"
namespace mineplus {
namespace {
constexpr SurfaceTradeOffer offers[5][SURFACE_TRADE_OFFERS]={
 {{27,8,"RUNE FRAGMENTS"},{29,15,"ANCIENT RELICS"},{30,25,"BOSS SIGILS"}},
 {{18,5,"IRON BARS"},{19,12,"OBSIDIAN CORES"},{20,8,"EMBER CATALYSTS"}},
 {{23,3,"HERBS"},{24,4,"BONE SHARDS"},{26,7,"CRYSTAL SHARDS"}},
 {{25,6,"DUNGEON ORE"},{26,5,"CRYSTAL SHARDS"},{27,9,"RUNE FRAGMENTS"}},
 {{24,3,"BONE SHARDS"},{28,12,"EMBER CORES"},{29,14,"ANCIENT RELICS"}}
};
}
SurfaceTradeOffer surface_trade_offer(std::uint8_t role,std::uint8_t offer){
 return offers[role%5u][offer%SURFACE_TRADE_OFFERS];
}
std::uint16_t surface_trade_price(const SurfaceTradeOffer& offer,std::uint8_t discount_percent){
 const unsigned discount=discount_percent>75u?75u:discount_percent;
 return static_cast<std::uint16_t>((unsigned(offer.coin_cost)*(100u-discount)+99u)/100u);
}
bool surface_trade_buy(std::uint8_t role,std::uint8_t offer,int* inventory,std::size_t count,std::uint8_t discount_percent){
 if(!inventory||count<31u||offer>=SURFACE_TRADE_OFFERS)return false;
 const auto item=surface_trade_offer(role,offer);
 constexpr int COINS=22;
 const auto price=surface_trade_price(item,discount_percent);
 if(inventory[COINS]<price||inventory[item.item_id]<0||inventory[item.item_id]>=9999)return false;
 inventory[COINS]-=price;
 ++inventory[item.item_id];
 return true;
}
} // namespace mineplus

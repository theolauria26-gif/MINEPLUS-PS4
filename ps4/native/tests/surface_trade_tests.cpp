#include "surface_trade.hpp"
#include <cassert>
#include <iostream>
int main(){
 using namespace mineplus;
 for(std::uint8_t role=0;role<5u;++role)for(std::uint8_t offer=0;offer<3u;++offer){
  const auto item=surface_trade_offer(role,offer);
  assert(item.name&&item.name[0]!='\0'&&item.item_id<31u&&item.coin_cost>0u);
 }
 int inventory[31]={};inventory[22]=20;
 const auto offer=surface_trade_offer(1,0);
 assert(surface_trade_price(offer,25)==(offer.coin_cost*75u+99u)/100u);
 assert(surface_trade_price(offer,100)==(offer.coin_cost*25u+99u)/100u);
 assert(surface_trade_buy(1,0,inventory,31u));
 assert(inventory[22]==20-int(offer.coin_cost)&&inventory[offer.item_id]==1);
 inventory[22]=20;
 assert(surface_trade_buy(1,0,inventory,31u,25u));
 assert(inventory[22]==20-int(surface_trade_price(offer,25)));
 const int items=inventory[offer.item_id];
 assert(!surface_trade_buy(1,0,inventory,30u));
 inventory[22]=0;
 assert(!surface_trade_buy(1,0,inventory,31u));
 assert(inventory[22]==0&&inventory[offer.item_id]==items);
 inventory[22]=20;inventory[offer.item_id]=9999;
 assert(!surface_trade_buy(1,0,inventory,31u));
 assert(inventory[22]==20&&inventory[offer.item_id]==9999);
 assert(!surface_trade_buy(0,3,inventory,31u));
 assert(!surface_trade_buy(0,0,nullptr,31u));
 std::cout<<"MINEPLUS surface trade tests: OK\n";
 return 0;
}

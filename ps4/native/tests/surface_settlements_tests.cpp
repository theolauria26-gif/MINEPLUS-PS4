#include "surface_settlements.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using namespace mineplus;
    PcgWorld a(4839201ULL), b(4839201ULL), other(998877ULL);
    unsigned found=0;
    for(std::int32_t y=-20;y<=20;++y) {
        for(std::int32_t x=-20;x<=20;++x) {
            SurfaceSettlement sa{},sb{},sc{};
            const bool ha=surface_settlement_for_cell(a,x,y,sa);
            const bool hb=surface_settlement_for_cell(b,x,y,sb);
            assert(ha==hb);
            if(!ha) continue;
            ++found;
            assert(sa.x==sb.x && sa.y==sb.y);
            assert(sa.layout==sb.layout && sa.resident_count==sb.resident_count);
            assert(sa.resident_count>=5u && sa.resident_count<=7u);
            for(std::uint8_t i=0;i<5;++i) {
                SettlementBuilding ba{},bb{};
                assert(surface_settlement_building(sa,i,ba));
                assert(surface_settlement_building(sb,i,bb));
                assert(ba.offset_x==bb.offset_x && ba.offset_y==bb.offset_y);
                assert(ba.half_extent>=3u && ba.half_extent<=4u);
                assert(ba.role<5u);
            }
            SettlementBuilding invalid{};
            assert(!surface_settlement_building(sa,5,invalid));
            assert(!surface_settlement_building(sa,255,invalid));
            (void)surface_settlement_for_cell(other,x,y,sc);
        }
    }
    assert(found>0u);
    SurfaceSettlement invalid{};
    assert(!surface_settlement_for_cell(a,INT32_MAX,INT32_MAX,invalid));
    std::cout << "MINEPLUS surface settlement tests: OK\n";
    return 0;
}

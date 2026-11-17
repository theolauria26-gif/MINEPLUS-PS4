#include "boss_combat.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace mineplus;

int main() {
    BossEncounter boss{};
    assert(!BossCombatDirector::start(boss, 0, 0));
    assert(BossCombatDirector::start(boss, 100, 1200));
    assert(boss.health == 1200 && boss.max_health == 1200);
    assert(boss.state == BossState::Dormant && boss.phase == 1);

    assert(BossCombatDirector::update(boss, 100) == BossEvent::TelegraphStarted);
    assert(boss.state == BossState::Telegraph);
    assert(boss.pending_attack != BossAttack::None);
    assert(BossCombatDirector::update(boss, 999) == BossEvent::None);
    assert(BossCombatDirector::update(boss, 1000) == BossEvent::AttackResolved);
    assert(boss.state == BossState::Recovering);
    assert(BossCombatDirector::update(boss, 2249) == BossEvent::None);
    assert(BossCombatDirector::update(boss, 2250) == BossEvent::TelegraphStarted);

    assert(BossCombatDirector::damage(boss, 407) == 407);
    assert(boss.health == 793 && boss.phase == 1);
    assert(BossCombatDirector::damage(boss, 1) == 1);
    assert(boss.phase == 1);
    // The phase change is announced when the current recovery ends.
    assert(BossCombatDirector::update(boss, 3150) == BossEvent::AttackResolved);
    assert(BossCombatDirector::update(boss, 4400) == BossEvent::PhaseChanged);
    assert(boss.phase == 2);
    assert(BossCombatDirector::damage(boss, 400) == 400);
    assert(boss.health == 392 && boss.phase == 2);
    assert(BossCombatDirector::update(boss, 5100) == BossEvent::AttackResolved);
    assert(BossCombatDirector::update(boss, 6350) == BossEvent::PhaseChanged);
    assert(boss.phase == 3);
    assert(BossCombatDirector::damage(boss, 10000) == 392);
    assert(boss.health == 0 && boss.state == BossState::Defeated);
    assert(BossCombatDirector::damage(boss, 1) == 0);
    assert(BossCombatDirector::update(boss, 999999) == BossEvent::None);
    assert(BossCombatDirector::claim_reward(boss));
    assert(!BossCombatDirector::claim_reward(boss));

    const auto slam = BossCombatDirector::attack_profile(BossAttack::GroundSlam, 3);
    assert(slam.damage == 6 && slam.range == 1 && slam.area_radius == 4);
    const auto volley = BossCombatDirector::attack_profile(BossAttack::RuneVolley, 2);
    assert(volley.damage == 4 && volley.range == 4 && volley.area_radius == 0);
    const auto charge = BossCombatDirector::attack_profile(BossAttack::RiftCharge, 1);
    assert(charge.damage == 5 && charge.range == 3 && charge.area_radius == 1);
    const auto none = BossCombatDirector::attack_profile(BossAttack::None, 1);
    assert(none.damage == 0 && none.range == 0 && none.area_radius == 0);
    assert(BossCombatDirector::attack_hits_player(slam, 0, 4));
    assert(!BossCombatDirector::attack_hits_player(slam, 3, 2));
    assert(BossCombatDirector::attack_hits_player(volley, 4, 0));
    assert(!BossCombatDirector::attack_hits_player(volley, 2, 2));
    assert(BossCombatDirector::attack_hits_player(charge, 4, 0));
    assert(!BossCombatDirector::attack_hits_player(charge, 4, 2));
    assert(!BossCombatDirector::attack_hits_player(none, 0, 0));

    BossEncounter wrap{};
    assert(BossCombatDirector::start(wrap, 0xfffffff0u, 100));
    assert(BossCombatDirector::update(wrap, 0xfffffff0u) == BossEvent::TelegraphStarted);
    assert(BossCombatDirector::update(wrap, 0x00000374u) == BossEvent::AttackResolved);

    BossEncounter small{};
    assert(BossCombatDirector::start(small, 0, 1));
    assert(BossCombatDirector::damage(small, 1) == 1);
    assert(small.state == BossState::Defeated);
    assert(BossCombatDirector::phase_for_health(0, 0) == 3);
    std::cout << "MINEPLUS boss combat tests: OK\n";
}

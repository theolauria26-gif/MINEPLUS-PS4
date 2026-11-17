#include "boss_combat.hpp"
#include <algorithm>
#include <limits>

namespace mineplus {
namespace {

bool elapsed(std::uint32_t now, std::uint32_t started, std::uint32_t duration) {
    return static_cast<std::uint32_t>(now - started) >= duration;
}

BossAttack attack_for(std::uint32_t index, std::uint8_t phase) {
    static constexpr BossAttack cycle[] = {
        BossAttack::GroundSlam, BossAttack::RuneVolley, BossAttack::RiftCharge
    };
    const std::uint32_t offset = phase >= 3 ? 2u : (phase == 2 ? 1u : 0u);
    return cycle[(index + offset) % 3u];
}

} // namespace

bool BossCombatDirector::start(BossEncounter& boss, std::uint32_t now_ms,
                               std::uint32_t max_health) {
    if (max_health == 0 || max_health > 100000000u) return false;
    BossEncounter candidate{};
    candidate.max_health = max_health;
    candidate.health = max_health;
    candidate.state_started_at = now_ms;
    candidate.state = BossState::Dormant;
    boss = candidate;
    return true;
}

std::uint32_t BossCombatDirector::damage(BossEncounter& boss, std::uint32_t amount) {
    if (boss.state == BossState::Defeated || boss.max_health == 0 ||
        boss.health == 0 || amount == 0) return 0;
    const std::uint32_t applied = std::min(amount, boss.health);
    boss.health -= applied;
    if (boss.health == 0) {
        boss.state = BossState::Defeated;
        boss.pending_attack = BossAttack::None;
        boss.state_duration_ms = 0;
    }
    return applied;
}

BossEvent BossCombatDirector::update(BossEncounter& boss, std::uint32_t now_ms,
                                     BossTuning tuning) {
    if (boss.max_health == 0 || boss.health == 0 ||
        boss.state == BossState::Defeated) return BossEvent::None;

    if (boss.state == BossState::Dormant) {
        boss.phase = phase_for_health(boss.health, boss.max_health);
        boss.pending_attack = attack_for(boss.attack_index, boss.phase);
        boss.state = BossState::Telegraph;
        boss.state_started_at = now_ms;
        boss.state_duration_ms = boss.phase == 1 ? tuning.telegraph_ms :
            (boss.phase == 2 ? tuning.phase_two_telegraph_ms : tuning.phase_three_telegraph_ms);
        return BossEvent::TelegraphStarted;
    }

    if (!elapsed(now_ms, boss.state_started_at, boss.state_duration_ms))
        return BossEvent::None;

    if (boss.state == BossState::Telegraph) {
        boss.state = BossState::Recovering;
        boss.state_started_at = now_ms;
        boss.state_duration_ms = tuning.recovery_ms;
        ++boss.attack_index;
        return BossEvent::AttackResolved;
    }

    if (boss.state == BossState::Recovering) {
        const std::uint8_t next_phase = phase_for_health(boss.health, boss.max_health);
        const bool changed = next_phase != boss.phase;
        boss.phase = next_phase;
        boss.pending_attack = attack_for(boss.attack_index, boss.phase);
        boss.state = BossState::Telegraph;
        boss.state_started_at = now_ms;
        boss.state_duration_ms = boss.phase == 1 ? tuning.telegraph_ms :
            (boss.phase == 2 ? tuning.phase_two_telegraph_ms : tuning.phase_three_telegraph_ms);
        return changed ? BossEvent::PhaseChanged : BossEvent::TelegraphStarted;
    }
    return BossEvent::None;
}

BossAttackProfile BossCombatDirector::attack_profile(BossAttack attack, std::uint8_t phase) {
    const std::uint8_t safe_phase = std::clamp<std::uint8_t>(phase, 1, 3);
    switch (attack) {
    case BossAttack::GroundSlam:
        return {attack, static_cast<std::uint16_t>(3u + safe_phase),
                1u, static_cast<std::uint16_t>(1u + safe_phase)};
    case BossAttack::RuneVolley:
        return {attack, static_cast<std::uint16_t>(2u + safe_phase),
                4u, 0u};
    case BossAttack::RiftCharge:
        return {attack, static_cast<std::uint16_t>(4u + safe_phase),
                3u, 1u};
    case BossAttack::None:
    default:
        return {};
    }
}

bool BossCombatDirector::attack_hits_player(const BossAttackProfile& profile,
                                             std::uint16_t dx, std::uint16_t dy) {
    const std::uint32_t distance = static_cast<std::uint32_t>(dx) + dy;
    switch (profile.attack) {
    case BossAttack::GroundSlam:
        return distance <= profile.area_radius;
    case BossAttack::RuneVolley:
        return distance <= profile.range && (dx == 0 || dy == 0);
    case BossAttack::RiftCharge:
        return distance <= static_cast<std::uint32_t>(profile.range) + profile.area_radius;
    case BossAttack::None:
    default:
        return false;
    }
}

bool BossCombatDirector::claim_reward(BossEncounter& boss) {
    if (boss.state != BossState::Defeated || boss.reward_claimed) return false;
    boss.reward_claimed = true;
    return true;
}

std::uint8_t BossCombatDirector::phase_for_health(std::uint32_t health,
                                                   std::uint32_t max_health) {
    if (max_health == 0 || health == 0) return 3;
    const std::uint64_t scaled = static_cast<std::uint64_t>(health) * 100u;
    if (scaled <= static_cast<std::uint64_t>(max_health) * 33u) return 3;
    if (scaled <= static_cast<std::uint64_t>(max_health) * 66u) return 2;
    return 1;
}

} // namespace mineplus

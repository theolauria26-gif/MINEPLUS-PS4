#pragma once
#include <cstdint>

namespace mineplus {

enum class BossState : std::uint8_t {
    Dormant = 0,
    Telegraph = 1,
    Recovering = 2,
    Defeated = 3
};

enum class BossAttack : std::uint8_t {
    None = 0,
    GroundSlam = 1,
    RuneVolley = 2,
    RiftCharge = 3
};

enum class BossEvent : std::uint8_t {
    None = 0,
    TelegraphStarted = 1,
    AttackResolved = 2,
    PhaseChanged = 3,
    Defeated = 4
};

struct BossEncounter {
    std::uint32_t max_health = 1200;
    std::uint32_t health = 1200;
    std::uint32_t state_started_at = 0;
    std::uint32_t state_duration_ms = 0;
    std::uint32_t attack_index = 0;
    std::uint8_t phase = 1;
    BossState state = BossState::Dormant;
    BossAttack pending_attack = BossAttack::None;
    bool reward_claimed = false;
};

struct BossAttackProfile {
    BossAttack attack = BossAttack::None;
    std::uint16_t damage = 0;
    std::uint8_t range = 0;
    std::uint16_t area_radius = 0;
};

struct BossTuning {
    std::uint32_t telegraph_ms = 900;
    std::uint32_t recovery_ms = 1250;
    std::uint32_t phase_two_telegraph_ms = 700;
    std::uint32_t phase_three_telegraph_ms = 520;
};

class BossCombatDirector {
public:
    static bool start(BossEncounter& boss, std::uint32_t now_ms,
                      std::uint32_t max_health = 1200);
    // Damage is clamped to remaining health. Returns the actual damage applied.
    static std::uint32_t damage(BossEncounter& boss, std::uint32_t amount);
    // Advances one deterministic attack cycle. Call once per game update.
    static BossEvent update(BossEncounter& boss, std::uint32_t now_ms,
                            BossTuning tuning = {});
    static bool claim_reward(BossEncounter& boss);
    static BossAttackProfile attack_profile(BossAttack attack, std::uint8_t phase);
    static bool attack_hits_player(const BossAttackProfile& profile,
                                   std::uint16_t dx, std::uint16_t dy);
    static std::uint8_t phase_for_health(std::uint32_t health,
                                         std::uint32_t max_health);
};

} // namespace mineplus

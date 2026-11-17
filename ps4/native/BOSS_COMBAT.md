# Boss combat director

The portable boss combat director provides a deterministic, testable encounter state machine for future in-world boss integration.

## Current mechanics

- Three health phases, with transitions at 66% and 33% remaining health.
- Three attack telegraphs: Ground Slam, Rune Volley, and Rift Charge.
- Telegraph duration tightens in later phases; recovery windows remain configurable.
- Attack selection rotates deterministically and changes its starting offset by phase.
- Damage is clamped to remaining health; defeated bosses cannot be damaged again.
- A reward can be claimed exactly once after defeat.
- Timer comparisons use unsigned elapsed-time subtraction, safe across the 32-bit millisecond counter wrap.

## Gameplay integration

The Primordial Guardian now occupies a deterministic dark-stone arena 16 tiles east of the starting clearing. Approaching within 9 Manhattan tiles activates its encounter. The SDL2 renderer draws a towering runic silhouette and a boss health bar; this is a projected 2D sprite treatment, not a GPU voxel model.

- Normal melee and weapon arts damage the Guardian when the player faces its arena center.
- Ground Slam resolves against its phase-scaled area radius, Rune Volley checks a straight line and range, and Rift Charge checks its phase-scaled reach.
- Telegraphs appear in the HUD. Resolved attacks apply elemental damage through equipped armor mitigation and durability.
- Defeat grants one Boss Sigil, three Ancient Relics and two Ember Cores. The reward-claimed flag prevents duplicate payout.
- Save format v11 persists boss health, phase, attack cycle, telegraph/recovery timing, pending attack and reward state. Versions 1–10 load a fresh dormant encounter; the arena is rebuilt after loading legacy worlds.

The encounter is integrated into SDL2 gameplay and saves, but it still has no skeletal animation, GPU mesh, simulated projectile objects or dedicated boss audio.

## Tests

`tests/boss_combat_tests.cpp` covers initialization, telegraph/recovery timing, deterministic attack cycles, phase thresholds, damage clamping, one-time rewards, defeat lockout, small health pools, and timer wraparound.

## Attack profiles

Each attack exposes a phase-scaled damage and targeting profile for the gameplay layer:

| Attack | Base damage by phase | Range | Area radius by phase |
|---|---:|---:|---:|
| Ground Slam | 4 / 5 / 6 | 1 tile | 2 / 3 / 4 tiles |
| Rune Volley | 3 / 4 / 5 | 4 tiles | Single-target line |
| Rift Charge | 5 / 6 / 7 | 3 tiles | 1 tile |

The gameplay layer resolves these profiles against the player's tile position. Projectiles and boss motion are still represented by range/area checks rather than simulated projectile objects.

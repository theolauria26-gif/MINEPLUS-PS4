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

## Integration status

This module is currently a portable combat director and is not yet connected to a visible boss entity, arena trigger, animation, hitbox, or save-game progression in the SDL2 gameplay. The next integration step must connect its events to boss movement/attacks, player damage and persistent dungeon completion. Do not treat this module alone as an in-game boss encounter.

## Tests

`tests/boss_combat_tests.cpp` covers initialization, telegraph/recovery timing, deterministic attack cycles, phase thresholds, damage clamping, one-time rewards, defeat lockout, small health pools, and timer wraparound.

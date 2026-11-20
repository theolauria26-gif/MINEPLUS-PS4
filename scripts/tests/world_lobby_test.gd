extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS LOBBY TEST FAILED: " + description)

func _run() -> void:
	_expect(MineplusWorldLobby.save_path_for_slot(1) == "user://mineplus_world.json", "slot one keeps the legacy save path")
	_expect(MineplusWorldLobby.save_path_for_slot(2) == "user://mineplus_world_2.json", "slot two has an isolated save path")
	_expect(MineplusWorldLobby.save_path_for_slot(3) == "user://mineplus_world_3.json", "slot three has an isolated save path")
	_expect(MineplusWorldLobby.save_path_for_slot(0) == MineplusWorldLobby.save_path_for_slot(1), "invalid low slot is clamped safely")
	_expect(MineplusWorldLobby.save_path_for_slot(99) == MineplusWorldLobby.save_path_for_slot(3), "invalid high slot is clamped safely")

	var seed_one := MineplusWorldLobby.default_world_seed(1)
	var seed_two := MineplusWorldLobby.default_world_seed(2)
	var seed_three := MineplusWorldLobby.default_world_seed(3)
	_expect(seed_one == 4839201, "slot one preserves the established default seed")
	_expect(seed_one != seed_two and seed_two != seed_three and seed_one != seed_three, "new slots receive distinct deterministic seeds")
	_expect(MineplusWorldLobby.default_world_seed(0) == seed_one, "seed selection clamps an invalid low slot")
	_expect(MineplusWorldLobby.default_world_seed(99) == seed_three, "seed selection clamps an invalid high slot")

	_expect(MineplusWorldLobby.slot_state(false, {}) == "empty", "missing save is offered as a new world")
	_expect(MineplusWorldLobby.slot_state(true, { "world_seed": 123 }) == "saved", "validated payload is offered as continue")
	_expect(MineplusWorldLobby.slot_state(true, {}) == "invalid", "existing invalid save is never treated as an empty slot")

	var packed := load("res://scenes/world_lobby.tscn") as PackedScene
	_expect(packed != null, "world lobby scene loads")
	if packed != null:
		var lobby := packed.instantiate()
		root.add_child(lobby)
		await process_frame
		_expect(lobby.get_child_count() >= 3, "world lobby builds its background, layout and slot cards")
		lobby.queue_free()
		await process_frame

	if failures == 0:
		print("MINEPLUS world-lobby tests: OK")
	else:
		push_error("MINEPLUS world-lobby tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

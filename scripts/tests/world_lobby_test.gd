extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS LOBBY TEST FAILED: " + description)

func _remove_test_file(path: String) -> void:
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))

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

	var temp_path := "user://mineplus_lobby_inspection_test.json"
	_remove_test_file(temp_path)
	_remove_test_file(temp_path + ".bak")
	_expect(MineplusWorldSave.save_world(temp_path, { "version": 7, "blocks": [], "world_seed": 12345 }), "lobby fixture save is created")
	var valid_info := MineplusWorldLobby.inspect_save_path(temp_path)
	_expect(valid_info.get("state", "") == "saved", "lobby recognizes a valid persisted world")
	_expect(valid_info.get("payload", {}).get("world_seed", 0) == 12345, "lobby reads the saved world's seed")

	var corrupt_file := FileAccess.open(temp_path, FileAccess.WRITE)
	_expect(corrupt_file != null, "corrupt save fixture can be opened")
	if corrupt_file != null:
		corrupt_file.store_string("{ broken")
		corrupt_file.close()
	var invalid_info := MineplusWorldLobby.inspect_save_path(temp_path)
	_expect(invalid_info.get("state", "") == "invalid", "corrupt save without backup is protected from overwrite")

	var backup_file := FileAccess.open(temp_path + ".bak", FileAccess.WRITE)
	_expect(backup_file != null, "valid backup fixture can be opened")
	if backup_file != null:
		backup_file.store_string(JSON.stringify({ "version": 7, "blocks": [], "world_seed": 54321 }))
		backup_file.close()
	var recovered_info := MineplusWorldLobby.inspect_save_path(temp_path)
	_expect(recovered_info.get("state", "") == "saved", "lobby accepts a valid backup after recovering it")
	_expect(recovered_info.get("payload", {}).get("world_seed", 0) == 54321, "recovered backup metadata is displayed")
	_expect(FileAccess.file_exists(temp_path), "backup recovery restores the primary save path")
	_remove_test_file(temp_path)
	_remove_test_file(temp_path + ".bak")
	_remove_test_file(temp_path + ".tmp")

	var packed := load("res://scenes/world_lobby.tscn") as PackedScene
	_expect(packed != null, "world lobby scene loads")
	if packed != null:
		var lobby := packed.instantiate()
		root.add_child(lobby)
		await process_frame
		var cards := lobby.find_child("WorldSlotCards", true, false)
		_expect(cards != null and cards.get_child_count() == 3, "world lobby builds exactly three selectable slot cards")
		lobby.queue_free()
		await process_frame

	if failures == 0:
		print("MINEPLUS world-lobby tests: OK")
	else:
		push_error("MINEPLUS world-lobby tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

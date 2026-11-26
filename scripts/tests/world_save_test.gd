extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS SAVE TEST FAILED: " + description)

func _remove_test_file(path: String) -> void:
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))

func _run() -> void:
	var path := "user://mineplus_world_save_test.json"
	_remove_test_file(path)
	_remove_test_file(path + ".bak")
	_remove_test_file(path + ".tmp")

	var payload := {
		"version": 4,
		"world_seed": 918273,
		"terrain_edits": [[-16, 32, 8, 3], ["bad", 0, 0, 0], [1, 2, 9999, 0]],
		"blocks": [[1, 2, 3, 5], [4, 5, 6, 99], ["bad", 0, 0, 1]],
		"inventory": [1200, -4, 12, 13, 14, 15],
		"pickaxe_durability": 99,
		"player_position": [12.5, 18.0, -7.25],
		"player_rotation_y": 1.25,
		"camera_pitch": 2.0,
		"selected_block": 9
	}
	_expect(MineplusWorldSave.save_world(path, payload), "valid save is written")
	var loaded := MineplusWorldSave.load_world(path)
	_expect(not loaded.is_empty(), "saved world can be read")
	_expect(int(loaded.get("version", 0)) == MineplusWorldSave.CURRENT_VERSION, "legacy payload migrates to current version")
	_expect(loaded.get("world_seed", 0) == 918273, "world seed survives round trip")
	_expect(loaded.get("blocks", []).size() == 1, "invalid placed-block entries are ignored")
	_expect(loaded.get("terrain_edits", []).size() == 1, "invalid terrain edits are ignored")
	_expect(loaded.get("inventory", [])[0] == 999 and loaded.get("inventory", [])[1] == 0, "inventory stacks are bounded")
	_expect(loaded.get("inventory", []).size() == 8 and loaded.get("inventory", [])[6] == 0 and loaded.get("inventory", [])[7] == 0, "legacy inventory migrates with empty wood and plank slots")
	_expect(loaded.get("axe_durability", -1) == 0, "legacy saves receive an empty axe slot")
	_expect(loaded.get("selected_tool", "invalid").is_empty(), "legacy saves begin with empty hands")
	_expect(loaded.get("pickaxe_durability", 0) == 40, "tool durability is bounded")
	_expect(loaded.get("camera_pitch", 0.0) == 1.45, "camera pitch is bounded")
	_expect(loaded.get("day_clock", -1.0) == 0.5, "legacy saves start at the default day-night phase")
	_expect(loaded.get("selected_block", 0) == 6, "selected hotbar slot is bounded to the new seventh block")

	var backup := FileAccess.open(path + ".bak", FileAccess.WRITE)
	_expect(backup != null, "backup fixture can be created")
	if backup != null:
		backup.store_string(JSON.stringify({
			"version": 5,
			"blocks": [],
			"terrain_edits": [],
			"inventory": [7, 8, 9, 10, 11, 12]
		}))
		backup.close()
	var corrupt := FileAccess.open(path, FileAccess.WRITE)
	_expect(corrupt != null, "corrupt fixture can be created")
	if corrupt != null:
		corrupt.store_string("{ interrupted json")
		corrupt.close()
	var recovered := MineplusWorldSave.load_world(path)
	_expect(not recovered.is_empty() and recovered.get("inventory", [])[0] == 7, "valid backup recovers a corrupt primary save")
	_expect(recovered.get("inventory", []).size() == 8 and recovered.get("axe_durability", -1) == 0, "version-five backup migrates inventory and tool fields")
	_expect(FileAccess.file_exists(path), "recovered backup is restored to the primary path")
	_expect(not MineplusWorldSave.save_world(path, {"version": 99, "blocks": []}), "future save versions are rejected")
	var migrated_seven := MineplusWorldSave.normalize_payload({"version": 7, "blocks": [], "inventory": [1, 2, 3, 4, 5, 6, 7], "selected_block": 6})
	_expect(migrated_seven.get("inventory", []).size() == 8 and migrated_seven.get("inventory", [])[6] == 7 and migrated_seven.get("inventory", [])[7] == 0, "version-seven saves gain an empty plank slot without losing wood")
	_expect(migrated_seven.get("selected_block", 0) == 6, "version-seven saves retain a valid seventh hotbar selection")
	var current_payload := {"version": 9, "blocks": [[1, 2, 3, 6]], "inventory": [1, 2, 3, 4, 5, 6, 7, 8], "axe_durability": 27, "selected_tool": "axe", "day_clock": 0.73, "selected_block": 6}
	_expect(MineplusWorldSave.save_world(path, current_payload), "version-nine inventory, plank blocks and tool state are saved")
	var current_loaded := MineplusWorldSave.load_world(path)
	_expect(current_loaded.get("inventory", []).size() == 8 and current_loaded.get("inventory", [])[6] == 7 and current_loaded.get("inventory", [])[7] == 8, "wood and plank stacks survive version-nine round trip")
	_expect(current_loaded.get("blocks", []).size() == 1 and current_loaded.get("blocks", [])[0][3] == 6, "placed plank block type survives version-nine round trip")
	_expect(current_loaded.get("selected_block", 0) == 6, "seventh hotbar slot survives version-nine round trip")
	_expect(current_loaded.get("axe_durability", 0) == 27, "axe durability survives version-nine round trip")
	_expect(current_loaded.get("selected_tool", "") == "axe", "selected axe survives version-nine round trip")
	var broken_tool_save := MineplusWorldSave.normalize_payload({"version": 9, "blocks": [], "axe_durability": 0, "selected_tool": "axe"})
	_expect(broken_tool_save.get("selected_tool", "invalid").is_empty(), "save normalization clears a selected tool with zero durability")
	var invalid_tool_save := MineplusWorldSave.normalize_payload({"version": 9, "blocks": [], "pickaxe_durability": 15, "selected_tool": "hammer"})
	_expect(invalid_tool_save.get("selected_tool", "invalid").is_empty(), "save normalization rejects unknown selected tools")
	var migrated_eight := MineplusWorldSave.normalize_payload({"version": 8, "blocks": [], "pickaxe_durability": 16, "axe_durability": 21})
	_expect(migrated_eight.get("selected_tool", "invalid").is_empty(), "version-eight saves migrate without unexpectedly equipping a tool")
	_expect(is_equal_approx(float(current_loaded.get("day_clock", 0.0)), 0.73), "day-night phase survives version-nine round trip")
	var migrated_six := MineplusWorldSave.normalize_payload({"version": 6, "blocks": [], "day_clock": 1.5})
	_expect(is_equal_approx(float(migrated_six.get("day_clock", 0.0)), 0.999999), "version-six saves migrate and clamp day-night phase")
	var invalid_clock := MineplusWorldSave.normalize_payload({"version": 8, "blocks": [], "day_clock": "night"})
	_expect(invalid_clock.get("day_clock", 0.0) == 0.5, "invalid day-night phase uses a safe default")

	_remove_test_file(path)
	_remove_test_file(path + ".bak")
	_remove_test_file(path + ".tmp")
	if failures == 0:
		print("MINEPLUS world-save tests: OK")
	else:
		push_error("MINEPLUS world-save tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

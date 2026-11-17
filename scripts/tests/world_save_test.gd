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
	_expect(loaded.get("pickaxe_durability", 0) == 40, "tool durability is bounded")
	_expect(loaded.get("camera_pitch", 0.0) == 1.45, "camera pitch is bounded")
	_expect(loaded.get("selected_block", 0) == 5, "selected hotbar slot is bounded")

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
	_expect(FileAccess.file_exists(path), "recovered backup is restored to the primary path")
	_expect(not MineplusWorldSave.save_world(path, {"version": 99, "blocks": []}), "future save versions are rejected")

	_remove_test_file(path)
	_remove_test_file(path + ".bak")
	_remove_test_file(path + ".tmp")
	if failures == 0:
		print("MINEPLUS world-save tests: OK")
	else:
		push_error("MINEPLUS world-save tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS SETTINGS TEST FAILED: " + description)

func _remove_test_file(path: String) -> void:
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))

func _run() -> void:
	var path := "user://mineplus_settings_test.cfg"
	_remove_test_file(path)
	var defaults := MineplusSettings.load_config(path)
	_expect(defaults != null, "missing settings file receives safe in-memory defaults")
	if defaults != null:
		_expect(defaults.get_value("graphics", "fps_limit", 0) == 60, "default frame cap is sixty FPS")
		_expect(defaults.get_value("world", "slot", 0) == 1, "default world slot remains one")

	_expect(MineplusSettings.save_values({
		"world": {"slot": 3},
		"hud": {"opacity": 0.7},
		"custom": {"preserved": "yes"}
	}, path) == OK, "initial settings patch is saved")
	_expect(MineplusSettings.save_values({
		"graphics": {"fov": 92.0, "quality": 2, "fps_limit": 30},
		"controls": {"sensitivity": 0.004},
		"appearance": {"skin": 2}
	}, path) == OK, "later settings patch is saved")
	var loaded := MineplusSettings.load_config(path)
	_expect(loaded != null, "settings file remains readable after multiple patches")
	if loaded != null:
		_expect(loaded.get_value("world", "slot", 0) == 3, "saving graphics does not reset selected world slot")
		_expect(is_equal_approx(float(loaded.get_value("hud", "opacity", 0.0)), 0.7), "saving controls preserves HUD opacity")
		_expect(loaded.get_value("custom", "preserved", "") == "yes", "unknown sections and keys are preserved")
		_expect(loaded.get_value("graphics", "fps_limit", 0) == 30, "frame cap survives persistence")
		_expect(MineplusSettings.fps_option_index(30) == 0 and MineplusSettings.fps_option_index(60) == 1 and MineplusSettings.fps_option_index(0) == 2, "FPS options map to stable UI entries")
		_expect(MineplusSettings.effective_fps_limit(0, true) == 60, "unlimited frame rate is capped on mobile")
		_expect(MineplusSettings.effective_fps_limit(30, true) == 30, "mobile power-saving cap remains thirty FPS")
		_expect(MineplusSettings.effective_fps_limit(60, false) == 60, "desktop balanced cap remains sixty FPS")
		_expect(MineplusSettings.effective_fps_limit(144, false) == 60, "unsupported frame-cap values fall back safely")
		var previous_limit := Engine.max_fps
		MineplusSettings.apply_runtime(loaded)
		_expect(Engine.max_fps == 30, "runtime frame pacing uses the saved cap")
		Engine.max_fps = previous_limit

	var corrupt := FileAccess.open(path, FileAccess.WRITE)
	_expect(corrupt != null, "corrupt settings fixture can be created")
	if corrupt != null:
		corrupt.store_string("[graphics\ninvalid")
		corrupt.close()
	_expect(MineplusSettings.load_config(path) == null, "malformed settings are reported instead of silently reset")
	_expect(MineplusSettings.save_values({"world": {"slot": 1}}, path) != OK, "saving refuses to overwrite malformed settings")
	var preserved := FileAccess.open(path, FileAccess.READ)
	if preserved != null:
		_expect(preserved.get_as_text() == "[graphics\ninvalid", "malformed settings remain untouched")
		preserved.close()

	_remove_test_file(path)
	if failures == 0:
		print("MINEPLUS settings tests: OK")
	else:
		push_error("MINEPLUS settings tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

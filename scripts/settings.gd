extends RefCounted
class_name MineplusSettings
## Leitura e gravação em patch: salvar uma opção nunca apaga outras seções do arquivo.

const SETTINGS_PATH := "user://mineplus_settings.cfg"

static func load_config(path: String = SETTINGS_PATH) -> ConfigFile:
	var config := ConfigFile.new()
	var result := config.load(path)
	if result != OK and result != ERR_FILE_NOT_FOUND:
		return null
	_ensure_defaults(config)
	return config

static func save_values(values: Dictionary, path: String = SETTINGS_PATH) -> int:
	var config := ConfigFile.new()
	var load_result := config.load(path)
	if load_result != OK and load_result != ERR_FILE_NOT_FOUND:
		return load_result
	for section_key in values.keys():
		var section_name := str(section_key)
		var section_values: Variant = values[section_key]
		if typeof(section_values) != TYPE_DICTIONARY:
			return ERR_INVALID_PARAMETER
		for value_key in section_values.keys():
			config.set_value(section_name, str(value_key), section_values[value_key])
	return config.save(path)

static func fps_option_index(limit: int) -> int:
	match limit:
		30: return 0
		0: return 2
		_: return 1

static func effective_fps_limit(requested: int, mobile: bool) -> int:
	var safe_limit := requested if requested == 0 or requested == 30 or requested == 60 else 60
	if mobile and safe_limit == 0:
		return 60
	return safe_limit

static func apply_runtime(config: ConfigFile) -> void:
	var requested := 60
	if config != null:
		requested = int(config.get_value("graphics", "fps_limit", 60))
	Engine.max_fps = effective_fps_limit(requested, OS.has_feature("mobile"))

static func _ensure_defaults(config: ConfigFile) -> void:
	var defaults := {
		"graphics": {"fov": 78.0, "quality": 1, "fps_limit": 60},
		"controls": {"sensitivity": 0.0025},
		"appearance": {"skin": 0},
		"hud": {"opacity": 1.0},
		"world": {"slot": 1}
	}
	for section_key in defaults.keys():
		var section_name := str(section_key)
		for value_key in defaults[section_key].keys():
			if not config.has_section_key(section_name, str(value_key)):
				config.set_value(section_name, str(value_key), defaults[section_key][value_key])

extends RefCounted
class_name MineplusWorldSave
## Versioned, bounded and recoverable world persistence for the Godot prototype.

const CURRENT_VERSION := 6
const MAX_SAVE_BYTES := 32 * 1024 * 1024
const MAX_WORLD_BLOCKS := 100000
const MAX_TERRAIN_EDITS := 250000
const MAX_COORDINATE := 10000000
const BLOCK_TYPE_COUNT := 6
const INVENTORY_ITEM_COUNT := 7
const TERRAIN_TYPE_COUNT := 11
const DEFAULT_INVENTORY := [64, 64, 32, 32, 16, 0, 0]
const DEFAULT_PLAYER_POSITION := [0.0, 13.0, 5.0]

static func _is_number(value: Variant) -> bool:
	var value_type := typeof(value)
	return (value_type == TYPE_INT or value_type == TYPE_FLOAT) and is_finite(float(value))

static func _valid_coordinate(value: Variant) -> bool:
	return _is_number(value) and absf(float(value)) <= float(MAX_COORDINATE)

static func _bounded_int(value: Variant, minimum: int, maximum: int, fallback: int) -> int:
	if not _is_number(value):
		return fallback
	return clampi(int(value), minimum, maximum)

static func normalize_payload(raw: Variant) -> Dictionary:
	if typeof(raw) != TYPE_DICTIONARY:
		return {}
	var source: Dictionary = raw
	var version_value: Variant = source.get("version", 1)
	if not _is_number(version_value):
		return {}
	var source_version := int(version_value)
	if source_version < 1 or source_version > CURRENT_VERSION:
		return {}

	var blocks_value: Variant = source.get("blocks", null)
	if typeof(blocks_value) != TYPE_ARRAY:
		return {}
	var source_blocks: Array = blocks_value
	if source_blocks.size() > MAX_WORLD_BLOCKS:
		return {}
	var blocks: Array = []
	for entry in source_blocks:
		if typeof(entry) != TYPE_ARRAY:
			continue
		var values: Array = entry
		if values.size() != 4:
			continue
		if not _valid_coordinate(values[0]) or not _valid_coordinate(values[1]) or not _valid_coordinate(values[2]):
			continue
		if not _is_number(values[3]):
			continue
		var block_type := int(values[3])
		if block_type < 0 or block_type >= BLOCK_TYPE_COUNT:
			continue
		blocks.append([int(values[0]), int(values[1]), int(values[2]), block_type])

	var edits_value: Variant = source.get("terrain_edits", [])
	if typeof(edits_value) != TYPE_ARRAY:
		return {}
	var source_edits: Array = edits_value
	if source_edits.size() > MAX_TERRAIN_EDITS:
		return {}
	var terrain_edits: Array = []
	for entry in source_edits:
		if typeof(entry) != TYPE_ARRAY:
			continue
		var values: Array = entry
		if values.size() != 4:
			continue
		if not _valid_coordinate(values[0]) or not _valid_coordinate(values[1]):
			continue
		if not _is_number(values[2]) or not _is_number(values[3]):
			continue
		var height := int(values[2])
		var terrain_type := int(values[3])
		if height < -128 or height > 512 or terrain_type < 0 or terrain_type >= TERRAIN_TYPE_COUNT:
			continue
		terrain_edits.append([int(values[0]), int(values[1]), height, terrain_type])

	var inventory := DEFAULT_INVENTORY.duplicate()
	var inventory_value: Variant = source.get("inventory", null)
	if typeof(inventory_value) == TYPE_ARRAY:
		var source_inventory: Array = inventory_value
		for index in range(mini(INVENTORY_ITEM_COUNT, source_inventory.size())):
			inventory[index] = _bounded_int(source_inventory[index], 0, 999, inventory[index])

	var player_position := DEFAULT_PLAYER_POSITION.duplicate()
	var position_value: Variant = source.get("player_position", null)
	if typeof(position_value) == TYPE_ARRAY:
		var position_parts: Array = position_value
		if position_parts.size() == 3 and _valid_coordinate(position_parts[0]) and _valid_coordinate(position_parts[1]) and _valid_coordinate(position_parts[2]):
			player_position = [float(position_parts[0]), float(position_parts[1]), float(position_parts[2])]

	var rotation_value: Variant = source.get("player_rotation_y", 0.0)
	var pitch_value: Variant = source.get("camera_pitch", 0.0)
	var seed_value: Variant = source.get("world_seed", 4839201)
	return {
		"version": CURRENT_VERSION,
		"world_seed": _bounded_int(seed_value, -2147483648, 2147483647, 4839201),
		"terrain_edits": terrain_edits,
		"blocks": blocks,
		"inventory": inventory,
		"pickaxe_durability": _bounded_int(source.get("pickaxe_durability", 0), 0, 40, 0),
		"axe_durability": _bounded_int(source.get("axe_durability", 0), 0, 40, 0),
		"player_position": player_position,
		"player_rotation_y": float(rotation_value) if _is_number(rotation_value) else 0.0,
		"camera_pitch": clampf(float(pitch_value), -1.45, 1.45) if _is_number(pitch_value) else 0.0,
		"selected_block": _bounded_int(source.get("selected_block", 0), 0, BLOCK_TYPE_COUNT - 1, 0)
	}

static func _absolute(path: String) -> String:
	return ProjectSettings.globalize_path(path)

static func _remove_if_present(path: String) -> void:
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(_absolute(path))

static func _read_valid_payload(path: String) -> Dictionary:
	if not FileAccess.file_exists(path):
		return {}
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		return {}
	var length := file.get_length()
	if length <= 0 or length > MAX_SAVE_BYTES:
		file.close()
		return {}
	var text := file.get_as_text()
	file.close()
	return normalize_payload(JSON.parse_string(text))

static func load_world(path: String) -> Dictionary:
	var primary := _read_valid_payload(path)
	if not primary.is_empty():
		return primary
	var backup_path := path + ".bak"
	var backup := _read_valid_payload(backup_path)
	if backup.is_empty():
		return {}
	# Recover a previous complete save if the latest file was interrupted or corrupted.
	_remove_if_present(path)
	DirAccess.rename_absolute(_absolute(backup_path), _absolute(path))
	return backup

static func save_world(path: String, payload: Dictionary) -> bool:
	var normalized := normalize_payload(payload)
	if normalized.is_empty():
		return false
	var serialized := JSON.stringify(normalized)
	if serialized.to_utf8_buffer().size() > MAX_SAVE_BYTES:
		return false

	var absolute_path := _absolute(path)
	var directory := absolute_path.get_base_dir()
	if not DirAccess.dir_exists_absolute(directory):
		if DirAccess.make_dir_recursive_absolute(directory) != OK:
			return false

	var temporary_path := path + ".tmp"
	var backup_path := path + ".bak"
	_remove_if_present(temporary_path)
	var file := FileAccess.open(temporary_path, FileAccess.WRITE)
	if file == null:
		return false
	file.store_string(serialized)
	file.flush()
	var write_error := file.get_error()
	file.close()
	if write_error != OK:
		_remove_if_present(temporary_path)
		return false

	var had_original := FileAccess.file_exists(path)
	if had_original:
		_remove_if_present(backup_path)
		if DirAccess.rename_absolute(absolute_path, _absolute(backup_path)) != OK:
			_remove_if_present(temporary_path)
			return false

	if DirAccess.rename_absolute(_absolute(temporary_path), absolute_path) != OK:
		if had_original and FileAccess.file_exists(backup_path):
			DirAccess.rename_absolute(_absolute(backup_path), absolute_path)
		_remove_if_present(temporary_path)
		return false

	_remove_if_present(backup_path)
	return true

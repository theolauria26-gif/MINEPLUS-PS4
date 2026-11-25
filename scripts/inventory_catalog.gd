extends RefCounted
class_name MineplusInventory
## Stable inventory metadata shared by the HUD and tests.

const CATEGORY_ALL := "all"
const CATEGORY_BLOCKS := "blocks"
const CATEGORY_MATERIALS := "materials"

const ITEM_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo", "Madeira", "Tábuas"]
const PLACEABLE_BLOCK_TYPES := [0, 1, 2, 3, 4, 5, -1, 6]

static func item_name(item_index: int) -> String:
	if item_index < 0 or item_index >= ITEM_NAMES.size():
		return ""
	return ITEM_NAMES[item_index]

static func is_placeable(item_index: int) -> bool:
	return item_index >= 0 and item_index < PLACEABLE_BLOCK_TYPES.size() and PLACEABLE_BLOCK_TYPES[item_index] >= 0

static func block_type_for_item(item_index: int) -> int:
	if item_index < 0 or item_index >= PLACEABLE_BLOCK_TYPES.size():
		return -1
	return PLACEABLE_BLOCK_TYPES[item_index]

static func category_title(category: String) -> String:
	match category:
		CATEGORY_BLOCKS:
			return "BLOCOS"
		CATEGORY_MATERIALS:
			return "RECURSOS"
		_:
			return "TODOS"

static func visible_item_indices(category: String) -> Array[int]:
	var result: Array[int] = []
	for item_index in range(ITEM_NAMES.size()):
		match category:
			CATEGORY_BLOCKS:
				if is_placeable(item_index):
					result.append(item_index)
			CATEGORY_MATERIALS:
				if not is_placeable(item_index):
					result.append(item_index)
			_:
				result.append(item_index)
	return result

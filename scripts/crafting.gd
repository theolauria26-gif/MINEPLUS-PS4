extends RefCounted
class_name MineplusCrafting
## Pure, transactional recipe rules shared by the crafting UI and automated tests.

const INVENTORY_ITEM_COUNT := 8
const MAX_STACK_SIZE := 999
const ITEM_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo", "Madeira", "Tábuas"]

const RECIPES := {
	"bricks": {
		"title": "Tijolos",
		"description": "2 Terra + 1 Pedra  →  2 Tijolos",
		"ingredients": [[1, 2], [2, 1]],
		"output_item": 5,
		"output_count": 2,
		"tool": ""
	},
	"planks": {
		"title": "Tábuas de madeira",
		"description": "1 Madeira  →  4 Tábuas",
		"ingredients": [[6, 1]],
		"output_item": 7,
		"output_count": 4,
		"tool": ""
	},
	"pickaxe": {
		"title": "Picareta",
		"description": "3 Pedras + 2 Tijolos  →  Picareta (40 usos)",
		"ingredients": [[2, 3], [5, 2]],
		"output_item": -1,
		"output_count": 0,
		"tool": "pickaxe"
	},
	"axe": {
		"title": "Machado de pedra",
		"description": "2 Pedras + 2 Terras  →  Machado (40 usos)",
		"ingredients": [[2, 2], [1, 2]],
		"output_item": -1,
		"output_count": 0,
		"tool": "axe"
	}
}

static func recipe_ids() -> Array[String]:
	return ["bricks", "planks", "pickaxe", "axe"]

static func inventory_index_for_block(block_type: int) -> int:
	# Raw wood is material 6; the seventh placeable block is crafted planks at item 7.
	if block_type < 0 or block_type >= 7:
		return -1
	return 7 if block_type == 6 else block_type

static func recipe_title(recipe_id: String) -> String:
	var recipe: Dictionary = RECIPES.get(recipe_id, {})
	return str(recipe.get("title", "Receita desconhecida"))

static func recipe_description(recipe_id: String) -> String:
	var recipe: Dictionary = RECIPES.get(recipe_id, {})
	return str(recipe.get("description", ""))

static func _failure(reason: String) -> Dictionary:
	return {"ok": false, "reason": reason}

static func can_craft(recipe_id: String, inventory: Array, pickaxe_durability: int, axe_durability: int) -> Dictionary:
	if not RECIPES.has(recipe_id):
		return _failure("Receita desconhecida.")
	if inventory.size() != INVENTORY_ITEM_COUNT:
		return _failure("Inventário incompatível.")
	for value in inventory:
		if typeof(value) != TYPE_INT and typeof(value) != TYPE_FLOAT:
			return _failure("Inventário contém uma pilha inválida.")
		var amount := float(value)
		if not is_finite(amount) or amount < 0.0 or amount > float(MAX_STACK_SIZE) or floor(amount) != amount:
			return _failure("Inventário contém uma quantidade inválida.")

	var recipe: Dictionary = RECIPES[recipe_id]
	var tool_id := str(recipe.get("tool", ""))
	if tool_id == "pickaxe" and pickaxe_durability > 0:
		return _failure("Você já possui uma picareta.")
	if tool_id == "axe" and axe_durability > 0:
		return _failure("Você já possui um machado.")

	for ingredient in recipe["ingredients"]:
		var item_index := int(ingredient[0])
		var required := int(ingredient[1])
		if item_index < 0 or item_index >= INVENTORY_ITEM_COUNT or required <= 0:
			return _failure("Receita contém um ingrediente inválido.")
		if int(inventory[item_index]) < required:
			return _failure("Faltam %d %s." % [required - int(inventory[item_index]), ITEM_NAMES[item_index]])

	var output_item := int(recipe.get("output_item", -1))
	var output_count := int(recipe.get("output_count", 0))
	if output_item >= 0:
		if output_item >= INVENTORY_ITEM_COUNT or output_count <= 0:
			return _failure("Receita contém uma saída inválida.")
		if int(inventory[output_item]) > MAX_STACK_SIZE - output_count:
			return _failure("Pilha de %s cheia." % ITEM_NAMES[output_item])

	return {"ok": true, "reason": "Disponível."}

static func craft(recipe_id: String, inventory: Array, pickaxe_durability: int, axe_durability: int) -> Dictionary:
	var availability := can_craft(recipe_id, inventory, pickaxe_durability, axe_durability)
	if not bool(availability.get("ok", false)):
		return availability

	var recipe: Dictionary = RECIPES[recipe_id]
	var next_inventory: Array = inventory.duplicate()
	for ingredient in recipe["ingredients"]:
		var item_index := int(ingredient[0])
		var required := int(ingredient[1])
		next_inventory[item_index] = int(next_inventory[item_index]) - required

	var next_pickaxe := pickaxe_durability
	var next_axe := axe_durability
	match str(recipe.get("tool", "")):
		"pickaxe":
			next_pickaxe = 40
		"axe":
			next_axe = 40
		_:
			var output_item := int(recipe["output_item"])
			next_inventory[output_item] = int(next_inventory[output_item]) + int(recipe["output_count"])

	return {
		"ok": true,
		"reason": "",
		"inventory": next_inventory,
		"pickaxe_durability": next_pickaxe,
		"axe_durability": next_axe
	}

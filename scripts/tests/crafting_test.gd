extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS CRAFTING TEST FAILED: " + description)

func _run() -> void:
	_expect(MineplusCrafting.recipe_ids().size() == 4, "the catalog exposes four integrated recipes")
	_expect(MineplusCrafting.recipe_description("axe").contains("2 Pedras"), "axe recipe displays its ingredient requirements")
	_expect(MineplusCrafting.recipe_description("planks").contains("4 Tábuas"), "plank recipe displays its output")
	_expect(MineplusCrafting.inventory_index_for_block(5) == 5, "brick blocks use the brick inventory stack")
	_expect(MineplusCrafting.inventory_index_for_block(6) == 7, "placeable planks use their crafted stack, not raw wood")
	_expect(MineplusCrafting.inventory_index_for_block(-1) == -1, "invalid block IDs cannot access inventory stacks")

	var insufficient := [64, 1, 0, 0, 0, 0, 0, 0]
	var insufficient_result := MineplusCrafting.craft("bricks", insufficient, 0, 0)
	_expect(not bool(insufficient_result.get("ok", true)), "missing ingredients reject a craft")
	_expect(insufficient == [64, 1, 0, 0, 0, 0, 0, 0], "failed crafting does not consume any item")

	var brick_inventory := [64, 4, 3, 0, 0, 0, 0, 0]
	var brick_result := MineplusCrafting.craft("bricks", brick_inventory, 0, 0)
	_expect(bool(brick_result.get("ok", false)), "brick recipe succeeds with exact ingredients")
	_expect(brick_result.get("inventory", []) == [64, 2, 2, 0, 0, 2, 0, 0], "brick recipe consumes two dirt and one stone, then creates two bricks")
	_expect(brick_inventory == [64, 4, 3, 0, 0, 0, 0, 0], "successful crafting returns a new inventory without mutating its input")

	var full_stack := [0, 2, 1, 0, 0, 999, 0, 0]
	var full_stack_result := MineplusCrafting.craft("bricks", full_stack, 0, 0)
	_expect(not bool(full_stack_result.get("ok", true)), "output stack overflow is rejected")
	_expect(full_stack == [0, 2, 1, 0, 0, 999, 0, 0], "full output stack does not consume ingredients")

	var plank_inventory := [0, 0, 0, 0, 0, 0, 2, 0]
	var plank_result := MineplusCrafting.craft("planks", plank_inventory, 0, 0)
	_expect(bool(plank_result.get("ok", false)), "plank recipe succeeds with raw wood")
	_expect(plank_result.get("inventory", []) == [0, 0, 0, 0, 0, 0, 1, 4], "plank recipe consumes one wood and creates four placeable planks")
	_expect(plank_inventory == [0, 0, 0, 0, 0, 0, 2, 0], "plank crafting does not mutate its source inventory")
	var full_plank_stack := [0, 0, 0, 0, 0, 0, 1, 999]
	var full_plank_result := MineplusCrafting.craft("planks", full_plank_stack, 0, 0)
	_expect(not bool(full_plank_result.get("ok", true)), "full plank stack rejects crafting")
	_expect(full_plank_stack[6] == 1 and full_plank_stack[7] == 999, "rejected plank crafting preserves wood and planks")

	var pickaxe_inventory := [0, 0, 3, 0, 0, 2, 0, 0]
	var pickaxe_result := MineplusCrafting.craft("pickaxe", pickaxe_inventory, 0, 0)
	_expect(bool(pickaxe_result.get("ok", false)), "pickaxe recipe succeeds")
	_expect(pickaxe_result.get("pickaxe_durability", 0) == 40, "crafted pickaxe receives forty uses")
	_expect(pickaxe_result.get("inventory", []) == [0, 0, 0, 0, 0, 0, 0, 0], "pickaxe recipe consumes three stone and two bricks")
	var duplicate_pickaxe := MineplusCrafting.craft("pickaxe", pickaxe_inventory, 12, 0)
	_expect(not bool(duplicate_pickaxe.get("ok", true)), "a second pickaxe cannot overwrite an existing tool")
	_expect(pickaxe_inventory == [0, 0, 3, 0, 0, 2, 0, 0], "duplicate tool rejection preserves all ingredients")

	var axe_inventory := [0, 2, 2, 0, 0, 0, 0, 0]
	var axe_result := MineplusCrafting.craft("axe", axe_inventory, 0, 0)
	_expect(bool(axe_result.get("ok", false)), "stone axe recipe succeeds")
	_expect(axe_result.get("axe_durability", 0) == 40, "crafted axe receives forty uses")
	_expect(axe_result.get("inventory", []) == [0, 0, 0, 0, 0, 0, 0, 0], "axe recipe consumes two stone and two dirt")

	var duplicate_axe := MineplusCrafting.craft("axe", axe_inventory, 0, 9)
	_expect(not bool(duplicate_axe.get("ok", true)), "a second axe cannot overwrite an existing tool")
	_expect(not bool(MineplusCrafting.can_craft("unknown", brick_inventory, 0, 0).get("ok", true)), "unknown recipe IDs are rejected")
	_expect(not bool(MineplusCrafting.can_craft("bricks", [1, 2], 0, 0).get("ok", true)), "short inventories are rejected safely")
	_expect(not bool(MineplusCrafting.can_craft("bricks", [1, -1, 1, 0, 0, 0, 0, 0], 0, 0).get("ok", true)), "negative inventory stacks are rejected")

	if failures == 0:
		print("MINEPLUS crafting tests: OK")
	else:
		push_error("MINEPLUS crafting tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

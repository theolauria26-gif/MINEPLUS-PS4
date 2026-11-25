extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS INVENTORY TEST FAILED: " + description)

func _run() -> void:
	_expect(MineplusInventory.visible_item_indices(MineplusInventory.CATEGORY_ALL) == [0, 1, 2, 3, 4, 5, 6, 7], "all category preserves the eight-slot inventory order")
	_expect(MineplusInventory.visible_item_indices(MineplusInventory.CATEGORY_BLOCKS) == [0, 1, 2, 3, 4, 5, 7], "blocks category excludes raw wood but includes crafted planks")
	_expect(MineplusInventory.visible_item_indices(MineplusInventory.CATEGORY_MATERIALS) == [6], "materials category exposes raw wood only")
	_expect(MineplusInventory.item_name(6) == "Madeira", "raw wood keeps its distinct inventory name")
	_expect(MineplusInventory.item_name(99).is_empty(), "invalid item IDs return no display name")
	_expect(MineplusInventory.is_placeable(7), "crafted planks are placeable")
	_expect(not MineplusInventory.is_placeable(6), "raw wood is not placeable")
	_expect(MineplusInventory.block_type_for_item(7) == 6, "plank item maps to the seventh placeable block")
	_expect(MineplusInventory.block_type_for_item(6) == -1, "raw wood cannot map to a placeable block")
	_expect(MineplusInventory.block_type_for_item(-1) == -1, "negative item IDs cannot equip a block")

	var world_script := load("res://scripts/world.gd") as GDScript
	_expect(world_script != null, "world script loads with the inventory catalog")
	if world_script != null:
		var world = world_script.new()
		world._set_inventory_filter(MineplusInventory.CATEGORY_BLOCKS)
		_expect(world.inventory_filter == MineplusInventory.CATEGORY_BLOCKS, "inventory UI accepts the blocks filter")
		world._set_inventory_filter("invalid")
		_expect(world.inventory_filter == MineplusInventory.CATEGORY_BLOCKS, "unknown inventory filters are ignored")
		world._equip_inventory_item(7)
		_expect(world.selected_block == 6, "selecting planks from inventory equips the matching hotbar block")
		world._equip_inventory_item(6)
		_expect(world.selected_block == 6, "raw wood cannot replace the selected placeable hotbar block")
		world.free()

	if failures == 0:
		print("MINEPLUS inventory tests: OK")
	else:
		push_error("MINEPLUS inventory tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

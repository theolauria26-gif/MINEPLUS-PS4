extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS WORLD SCENE TEST FAILED: " + description)

func _run() -> void:
	var packed := load("res://scenes/world.tscn") as PackedScene
	_expect(packed != null, "world scene can be loaded as a packed scene")
	if packed != null:
		var world: Node = packed.instantiate()
		world.set("world_slot", 99999)
		root.add_child(world)
		await process_frame
		_expect(world.get("player") != null, "world scene creates the first-person player")
		_expect(world.get("chunk_streamer") != null, "world scene creates its chunk streamer")
		_expect(world.get("inventory_panel") != null, "world scene creates the inventory panel")
		_expect(world.get("inventory_hand_button") != null, "inventory exposes the empty-hand selection")
		_expect(world.get("inventory_pickaxe_button") != null, "inventory exposes the pickaxe selection")
		_expect(world.get("inventory_axe_button") != null, "inventory exposes the axe selection")
		var pickaxe: Node3D = world.get("pickaxe_visual")
		var axe: Node3D = world.get("axe_visual")
		_expect(pickaxe != null and axe != null, "both first-person tool models are constructed")
		if pickaxe != null and axe != null:
			world.set("pickaxe_durability", 40)
			world.set("axe_durability", 40)
			world.set("selected_tool", MineplusEquipment.TOOL_PICKAXE)
			world.call("_update_tool_visuals")
			_expect(pickaxe.visible and not axe.visible, "only the selected pickaxe is shown")
			world.set("selected_tool", MineplusEquipment.TOOL_AXE)
			world.call("_update_tool_visuals")
			_expect(axe.visible and not pickaxe.visible, "switching selection shows only the axe")
			world.set("selected_tool", MineplusEquipment.TOOL_NONE)
			world.call("_update_tool_visuals")
			_expect(not axe.visible and not pickaxe.visible, "empty hands hide both tool models")
		world.queue_free()
		await process_frame

	if failures == 0:
		print("MINEPLUS world-scene smoke tests: OK")
	else:
		push_error("MINEPLUS WORLD SCENE TEST FAILED: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

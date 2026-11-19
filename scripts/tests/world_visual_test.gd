extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS VISUAL TEST FAILED: " + description)

func _run() -> void:
	var wind_shader := load("res://shaders/foliage_wind.gdshader") as Shader
	var world_script := load("res://scripts/world.gd") as GDScript
	_expect(world_script != null, "world input script loads for touch-zone tests")
	if world_script != null:
		_expect(world_script._touch_starts_look(Vector2(900, 320), Vector2(1280, 720)), "right-side upper touch starts camera look")
		_expect(not world_script._touch_starts_look(Vector2(600, 320), Vector2(1280, 720)), "left movement zone does not start camera look")
		_expect(not world_script._touch_starts_look(Vector2(900, 620), Vector2(1280, 720)), "lower action-button zone does not start camera look")
		_expect(not world_script._touch_starts_look(Vector2(900, -1), Vector2(1280, 720)), "touch outside viewport is rejected")
		_expect(not world_script._touch_starts_look(Vector2(900, 320), Vector2.ZERO), "zero-sized viewport is rejected")
		_expect(world_script._touch_drag_matches(2, 2), "active look finger can rotate the camera")
		_expect(not world_script._touch_drag_matches(2, 3), "a second finger cannot rotate the camera")
		_expect(not world_script._touch_drag_matches(-1, 0), "unassigned touch cannot rotate the camera")
	_expect(wind_shader != null, "procedural foliage wind shader loads")
	var streamer := MineplusChunkStreamer.new()
	streamer.generator = MineplusWorldGen.new(731942)
	var keys := [Vector2i(0, 0), Vector2i(-1, 0), Vector2i(38, 42), Vector2i(-46, 31)]
	for key in keys:
		var first := streamer.decoration_anchors(key)
		var second := streamer.decoration_anchors(key)
		_expect(first == second, "decoration anchors remain deterministic for chunk %s" % [key])
		var owned: Dictionary = {}
		for anchor in first:
			var ax := int(anchor.x)
			var az := int(anchor.z)
			_expect(ax >= key.x * 16 and ax < (key.x + 1) * 16, "anchor X belongs to its chunk")
			_expect(az >= key.y * 16 and az < (key.y + 1) * 16, "anchor Z belongs to its chunk")
			var position := Vector2i(ax, az)
			_expect(not owned.has(position), "chunk contains no duplicate decoration anchors")
			owned[position] = true
			var kind := int(anchor.kind)
			var biome := streamer.generator.biome_at(ax, az)
			if kind == 3:
				_expect(biome == MineplusWorldGen.Biome.TUNDRA, "conifers only spawn in tundra")
			elif kind == 4:
				_expect(biome == MineplusWorldGen.Biome.SWAMP, "twisted trees only spawn in swamps")
			elif kind == 5:
				_expect(biome == MineplusWorldGen.Biome.FOREST or biome == MineplusWorldGen.Biome.HIGHLANDS, "monoliths use ancient forest/highland biomes")

	for kind in [1, 2, 3, 4]:
		var tree_tool := SurfaceTool.new()
		tree_tool.begin(Mesh.PRIMITIVE_TRIANGLES)
		streamer._append_tree(tree_tool, {"x": 2, "y": 12, "z": -3, "kind": kind, "hash": 918273})
		var tree_mesh := tree_tool.commit()
		_expect(tree_mesh != null, "tree variant %d creates a 3D mesh" % kind)
	var ruin_tool := SurfaceTool.new()
	ruin_tool.begin(Mesh.PRIMITIVE_TRIANGLES)
	streamer._append_ruin(ruin_tool, {"x": 4, "y": 10, "z": 8, "kind": 6, "hash": 918273})
	var ruin_mesh := ruin_tool.commit()
	_expect(ruin_mesh != null, "ruined keep creates a 3D mesh")
	var gate_tool := SurfaceTool.new()
	gate_tool.begin(Mesh.PRIMITIVE_TRIANGLES)
	streamer._append_dungeon_gate(gate_tool, {"x": -4, "y": 10, "z": 8, "kind": 7, "hash": 918273})
	var gate_mesh := gate_tool.commit()
	_expect(gate_mesh != null, "dungeon gate creates a 3D mesh")

	if failures == 0:
		print("MINEPLUS world-visual tests: OK")
	else:
		push_error("MINEPLUS world-visual tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

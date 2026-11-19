extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS VISUAL TEST FAILED: " + description)

func _run() -> void:
	var streamer := MineplusChunkStreamer.new()
	streamer.generator = MineplusWorldGen.new(731942)
	root.add_child(streamer)

	var found_kinds: Dictionary = {}
	for z in range(-14, 15):
		for x in range(-14, 15):
			var key := Vector2i(x, z)
			var first := streamer.decoration_anchors(key)
			if posmod(x * 13 + z * 7, 5) == 0:
				var second := streamer.decoration_anchors(key)
				_expect(first == second, "decoration anchors remain deterministic for chunk %s" % [key])
			var owned: Dictionary = {}
			for anchor in first:
				var ax := int(anchor.x)
				var az := int(anchor.z)
				_expect(ax >= x * 16 and ax < (x + 1) * 16, "anchor X belongs to its chunk")
				_expect(az >= z * 16 and az < (z + 1) * 16, "anchor Z belongs to its chunk")
				var position := Vector2i(ax, az)
				_expect(not owned.has(position), "chunk contains no duplicate decoration anchors")
				owned[position] = true
				var kind := int(anchor.kind)
				found_kinds[kind] = true
				var biome := streamer.generator.biome_at(ax, az)
				if kind == 3:
					_expect(biome == MineplusWorldGen.Biome.TUNDRA, "conifers only spawn in tundra")
				elif kind == 4:
					_expect(biome == MineplusWorldGen.Biome.SWAMP, "twisted trees only spawn in swamps")
				elif kind == 5:
					_expect(biome == MineplusWorldGen.Biome.FOREST or biome == MineplusWorldGen.Biome.HIGHLANDS, "monoliths use ancient forest/highland biomes")

	for kind in [1, 2, 3, 4]:
		var tool := SurfaceTool.new()
		tool.begin(Mesh.PRIMITIVE_TRIANGLES)
		streamer._append_tree(tool, {"x": 2, "y": 12, "z": -3, "kind": kind, "hash": 918273})
		var mesh := tool.commit()
		_expect(mesh != null and mesh.get_surface_count() == 1, "tree kind %d produces real 3D triangle geometry" % kind)
	var monolith_tool := SurfaceTool.new()
	monolith_tool.begin(Mesh.PRIMITIVE_TRIANGLES)
	streamer._append_monolith(monolith_tool, {"x": 0, "y": 10, "z": 0})
	var monolith_mesh := monolith_tool.commit()
	_expect(monolith_mesh != null and monolith_mesh.get_surface_count() == 1, "monolith produces real 3D triangle geometry")

	_expect(found_kinds.has(1) and found_kinds.has(3) and found_kinds.has(4) and found_kinds.has(5), "test seed covers broadleaf, conifer, swamp and monolith variants")
	streamer.free()
	if failures == 0:
		print("MINEPLUS world-visual tests: OK")
	else:
		push_error("MINEPLUS world-visual tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

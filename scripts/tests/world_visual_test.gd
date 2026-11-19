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

	if failures == 0:
		print("MINEPLUS world-visual tests: OK")
	else:
		push_error("MINEPLUS world-visual tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

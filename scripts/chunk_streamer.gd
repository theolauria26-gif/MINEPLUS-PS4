extends Node3D
class_name MineplusChunkStreamer
## Chunks de superfície. A geração é determinística; só os chunks próximos existem na cena.
## Cada chunk usa uma malha e um collider, não um StaticBody por bloco.

const CHUNK_SIZE := 16
const LOAD_RADIUS := 3
const UNLOAD_RADIUS := LOAD_RADIUS + 1
const BLOCK_SIZE := 1.0

var generator := MineplusWorldGen.new()
var player: Node3D
var chunks: Dictionary = {}
var chunk_pool: Array[Node3D] = []
const MAX_POOLED_CHUNKS := 12
var pending_chunks: Array[Vector2i] = []
var queued_chunks: Dictionary = {}
var terrain_edits: Dictionary = {}
var material: StandardMaterial3D

func _ready() -> void:
	material = StandardMaterial3D.new()
	material.vertex_color_use_as_albedo = true
	material.roughness = 0.94
	set_process(true)

func _process(_delta: float) -> void:
	if player == null:
		return
	var center := _chunk_of_world(player.global_position)
	var wanted: Dictionary = {}
	var candidates: Array[Vector2i] = []
	for z in range(center.y - LOAD_RADIUS, center.y + LOAD_RADIUS + 1):
		for x in range(center.x - LOAD_RADIUS, center.x + LOAD_RADIUS + 1):
			var key := Vector2i(x, z)
			wanted[key] = true
			if not chunks.has(key) and not queued_chunks.has(key):
				candidates.append(key)
	candidates.sort_custom(func(a: Vector2i, b: Vector2i) -> bool:
		return a.distance_squared_to(center) < b.distance_squared_to(center)
	)
	for key in candidates:
		pending_chunks.append(key)
		queued_chunks[key] = true
	for key in chunks.keys():
		if not wanted.has(key) and maxi(abs(key.x - center.x), abs(key.y - center.y)) > UNLOAD_RADIUS:
			var node: Node3D = chunks[key]
			chunks.erase(key)
			_recycle_chunk(node)
	# Budgeta a criação: no máximo um chunk por frame para evitar um pico de FPS.
	if not pending_chunks.is_empty():
		var next_key: Vector2i = pending_chunks.pop_front()
		queued_chunks.erase(next_key)
		if wanted.has(next_key) and not chunks.has(next_key):
			_build_chunk(next_key)
	# Descarta trabalho obsoleto quando o jogador atravessa o mapa rapidamente.
	var keep_queue: Array[Vector2i] = []
	for pending in pending_chunks:
		if maxi(abs(pending.x - center.x), abs(pending.y - center.y)) <= UNLOAD_RADIUS:
			keep_queue.append(pending)
		else:
			queued_chunks.erase(pending)
	pending_chunks = keep_queue

func _chunk_of_world(position: Vector3) -> Vector2i:
	return Vector2i(floori(position.x / CHUNK_SIZE), floori(position.z / CHUNK_SIZE))

func _sample_world(x: int, z: int) -> Array:
	var sample: Array = generator.surface_at(x, z)
	var key := "%d,%d" % [x, z]
	if terrain_edits.has(key):
		var edit: Dictionary = terrain_edits[key]
		sample[0] = int(edit.height)
		sample[1] = int(edit.type)
	return sample

func _height(x: int, z: int) -> int:
	return int(_sample_world(x, z)[0])

func _block_type(x: int, z: int) -> int:
	return int(_sample_world(x, z)[1])

func _biome_color(biome: int, block_type: int) -> Color:
	match block_type:
		MineplusWorldGen.Block.WATER: return Color("#477f9b")
		MineplusWorldGen.Block.SAND: return Color("#c5ad72")
		MineplusWorldGen.Block.SNOW: return Color("#d8e1e5")
		MineplusWorldGen.Block.BASALT: return Color("#514b55")
		MineplusWorldGen.Block.STONE: return Color("#858b96")
		MineplusWorldGen.Block.WOOD: return Color("#765039")
		MineplusWorldGen.Block.BRICK: return Color("#98765f")
		MineplusWorldGen.Block.MOSS: return Color("#4e9d83")
	return Color("#79b85a") if biome != MineplusWorldGen.Biome.SWAMP else Color("#547d68")

func _add_quad(st: SurfaceTool, a: Vector3, b: Vector3, c: Vector3, d: Vector3, color: Color) -> void:
	var normal := (b - a).cross(c - a).normalized()
	st.set_color(color); st.set_normal(normal); st.add_vertex(a)
	st.set_color(color); st.set_normal(normal); st.add_vertex(b)
	st.set_color(color); st.set_normal(normal); st.add_vertex(c)
	st.set_color(color); st.set_normal(normal); st.add_vertex(a)
	st.set_color(color); st.set_normal(normal); st.add_vertex(c)
	st.set_color(color); st.set_normal(normal); st.add_vertex(d)

func _add_box(st: SurfaceTool, center: Vector3, size: Vector3, color: Color) -> void:
	var h := size * 0.5
	var x0 := center.x - h.x
	var x1 := center.x + h.x
	var y0 := center.y - h.y
	var y1 := center.y + h.y
	var z0 := center.z - h.z
	var z1 := center.z + h.z
	# Six outward-facing quads; decorations share one batched mesh per chunk.
	_add_quad(st, Vector3(x0,y1,z0), Vector3(x0,y1,z1), Vector3(x1,y1,z1), Vector3(x1,y1,z0), color)
	_add_quad(st, Vector3(x0,y0,z1), Vector3(x0,y0,z0), Vector3(x1,y0,z0), Vector3(x1,y0,z1), color.darkened(0.25))
	_add_quad(st, Vector3(x0,y0,z0), Vector3(x1,y0,z0), Vector3(x1,y1,z0), Vector3(x0,y1,z0), color.darkened(0.22))
	_add_quad(st, Vector3(x1,y0,z1), Vector3(x0,y0,z1), Vector3(x0,y1,z1), Vector3(x1,y1,z1), color.lightened(0.04))
	_add_quad(st, Vector3(x0,y0,z1), Vector3(x0,y0,z0), Vector3(x0,y1,z0), Vector3(x0,y1,z1), color.darkened(0.18))
	_add_quad(st, Vector3(x1,y0,z0), Vector3(x1,y0,z1), Vector3(x1,y1,z1), Vector3(x1,y1,z0), color.darkened(0.12))

func decoration_anchors(key: Vector2i) -> Array[Dictionary]:
	# One stable candidate per 8x8 macro-cell; chunk ownership prevents duplicates.
	var result: Array[Dictionary] = []
	var start_x := key.x * CHUNK_SIZE
	var start_z := key.y * CHUNK_SIZE
	var min_mx := floori(float(start_x) / 8.0)
	var max_mx := floori(float(start_x + CHUNK_SIZE - 1) / 8.0)
	var min_mz := floori(float(start_z) / 8.0)
	var max_mz := floori(float(start_z + CHUNK_SIZE - 1) / 8.0)
	for mz in range(min_mz, max_mz + 1):
		for mx in range(min_mx, max_mx + 1):
			var h := MineplusWorldGen._hash2(mx, mz, generator.world_seed ^ 0x4D1F)
			var x := mx * 8 + int((h >> 8) % 8)
			var z := mz * 8 + int((h >> 16) % 8)
			if x < start_x or x >= start_x + CHUNK_SIZE or z < start_z or z >= start_z + CHUNK_SIZE:
				continue
			var sample := _sample_world(x, z)
			var biome := int(sample[2])
			var roll := h % 100
			var kind := 0
			match biome:
				MineplusWorldGen.Biome.FOREST:
					if roll < 34: kind = 2 if h % 17 == 0 else 1
					elif roll >= 91: kind = 5
				MineplusWorldGen.Biome.TUNDRA:
					if roll < 27: kind = 3
				MineplusWorldGen.Biome.SWAMP:
					if roll < 31: kind = 4
				MineplusWorldGen.Biome.PLAINS:
					if roll < 7: kind = 1
				MineplusWorldGen.Biome.HIGHLANDS:
					if roll < 12: kind = 5
			if kind != 0:
				result.append({"x": x, "z": z, "y": int(sample[0]), "kind": kind, "hash": h})
	# Promote the original PCG's giant trees, ruins and dungeon arches into 3D
	# silhouettes. A structure belongs to exactly one chunk even when its mesh
	# extends a few blocks beyond that chunk's edge.
	var min_sx := floori(float(start_x) / generator.STRUCTURE_CELL)
	var max_sx := floori(float(start_x + CHUNK_SIZE - 1) / generator.STRUCTURE_CELL)
	var min_sz := floori(float(start_z) / generator.STRUCTURE_CELL)
	var max_sz := floori(float(start_z + CHUNK_SIZE - 1) / generator.STRUCTURE_CELL)
	for sz in range(min_sz, max_sz + 1):
		for sx in range(min_sx, max_sx + 1):
			var structure := generator._structure_anchor(sx, sz)
			if structure.is_empty():
				continue
			var ax := int(structure.x)
			var az := int(structure.z)
			if floori(float(ax) / CHUNK_SIZE) != key.x or floori(float(az) / CHUNK_SIZE) != key.y:
				continue
			var structure_kind := 2
			match int(structure.type):
				1: structure_kind = 2
				2: structure_kind = 6
				3: structure_kind = 7
			var structure_hash := MineplusWorldGen._hash2(sx, sz, generator.world_seed ^ 0x73A1)
			result.append({"x": ax, "z": az, "y": generator.ground_height_at(ax, az), "kind": structure_kind, "hash": structure_hash})
	return result

func _append_tree(st: SurfaceTool, anchor: Dictionary) -> void:
	var x := float(anchor.x)
	var y := float(anchor.y)
	var z := float(anchor.z)
	var kind := int(anchor.kind)
	var h := int(anchor.hash)
	var bark := Color("#514033") if kind == 4 else Color("#694b37")
	if kind == 2: bark = Color("#443b36")
	_add_box(st, Vector3(x, y + 1.9, z), Vector3(0.72, 3.8, 0.72), bark)
	# Exposed roots make the trunk feel anchored in the terrain.
	for side in [-1.0, 1.0]:
		_add_box(st, Vector3(x + side * 0.48, y + 0.22, z), Vector3(1.15, 0.32, 0.34), bark.darkened(0.12))
		_add_box(st, Vector3(x, y + 0.22, z + side * 0.48), Vector3(0.34, 0.32, 1.15), bark.darkened(0.12))
	if kind == 3:
		# Layered cold conifer: stacked, tapering dark evergreen boughs.
		for tier in range(4):
			var radius := 1.9 - float(tier) * 0.42
			var level := y + 2.2 + float(tier) * 1.05
			var shade := Color("#526d68").lightened(float((h >> (tier * 3)) & 3) * 0.025)
			_add_box(st, Vector3(x, level, z), Vector3(radius * 2.0, 1.25, radius * 2.0), shade)
			_add_box(st, Vector3(x, level + 0.48, z), Vector3(radius * 1.35, 0.72, radius * 1.35), shade.lightened(0.035))
		return
	if kind == 4:
		# Swamp tree: crooked, staggered branches and sparse hanging moss.
		_add_box(st, Vector3(x - 0.55, y + 3.5, z), Vector3(0.55, 0.55, 0.55), bark)
		_add_box(st, Vector3(x + 0.48, y + 4.05, z + 0.12), Vector3(0.5, 0.5, 0.5), bark)
		var moss := Color("#526f58")
		_add_box(st, Vector3(x - 0.72, y + 3.0, z), Vector3(0.72, 1.5, 0.72), moss)
		_add_box(st, Vector3(x + 0.62, y + 3.5, z + 0.12), Vector3(0.68, 1.4, 0.68), moss.darkened(0.12))
		return
	var canopy := Color("#355d43") if kind != 2 else Color("#304c3c")
	if kind == 2: canopy = Color("#3d5944")
	# Distinct cubic layers create a readable broadleaf/ancient silhouette.
	_add_box(st, Vector3(x, y + 4.0, z), Vector3(3.5, 2.0, 3.5), canopy)
	_add_box(st, Vector3(x - 1.15, y + 3.55, z + 0.12), Vector3(1.75, 1.55, 1.8), canopy.lightened(0.07))
	_add_box(st, Vector3(x + 1.12, y + 3.72, z - 0.18), Vector3(1.8, 1.65, 1.7), canopy.darkened(0.08))
	_add_box(st, Vector3(x, y + 5.15, z), Vector3(2.2, 1.55, 2.2), canopy.lightened(0.045))
	if kind == 2:
		_add_box(st, Vector3(x, y + 6.15, z), Vector3(1.25, 1.1, 1.25), Color("#53664a"))
	# A few world-stable moss patches break up the flat canopy palette.
	for i in range(3):
		if ((h >> (i * 5)) & 3) == 0:
			var side := -1.0 if i == 0 else (1.0 if i == 1 else 0.0)
			_add_box(st, Vector3(x + side * 1.15, y + 4.85, z + (1.0 - absf(side)) * 0.9), Vector3(0.62, 0.34, 0.62), Color("#71805a"))

func _append_monolith(st: SurfaceTool, anchor: Dictionary) -> void:
	var x := float(anchor.x)
	var y := float(anchor.y)
	var z := float(anchor.z)
	var stone := Color("#55515d")
	_add_box(st, Vector3(x, y + 1.55, z), Vector3(0.9, 3.1, 0.9), stone)
	_add_box(st, Vector3(x, y + 3.18, z), Vector3(1.12, 0.28, 1.12), Color("#77717a"))
	_add_box(st, Vector3(x, y + 3.48, z), Vector3(0.42, 0.48, 0.42), Color("#67b6a0"))

func _append_ruin(st: SurfaceTool, anchor: Dictionary) -> void:
	var x := float(anchor.x)
	var y := float(anchor.y)
	var z := float(anchor.z)
	var stone := Color("#625d61")
	var worn := Color("#81766d")
	# Broken low walls leave several readable gaps instead of a solid box.
	for side in [-1.0, 1.0]:
		_add_box(st, Vector3(x + side * 4.0, y + 1.35, z - 2.15), Vector3(0.7, 2.7, 2.5), stone)
		_add_box(st, Vector3(x + side * 4.0, y + 1.05, z + 2.25), Vector3(0.7, 2.1, 2.2), stone.darkened(0.12))
		_add_box(st, Vector3(x - 2.1, y + 1.25, z + side * 4.0), Vector3(2.5, 2.5, 0.7), worn)
		_add_box(st, Vector3(x + 2.25, y + 0.95, z + side * 4.0), Vector3(2.1, 1.9, 0.7), stone)
	# Four irregular corner remnants suggest collapsed towers.
	for sx in [-1.0, 1.0]:
		for sz in [-1.0, 1.0]:
			var height := 2.8 + float((int(anchor.hash) >> (int(sx + 1.0) * 2 + int(sz + 1.0))) & 3) * 0.38
			_add_box(st, Vector3(x + sx * 4.0, y + height * 0.5, z + sz * 4.0), Vector3(0.95, height, 0.95), worn)
	# Cracked flagstones and a rune plinth form a focal point in the courtyard.
	for dx in [-1.5, 0.0, 1.5]:
		for dz in [-1.5, 0.0, 1.5]:
			if absf(dx) + absf(dz) > 2.7:
				continue
			_add_box(st, Vector3(x + dx, y + 0.12, z + dz), Vector3(1.35, 0.22, 1.35), stone.darkened(0.16))
	_add_box(st, Vector3(x, y + 0.42, z), Vector3(1.35, 0.5, 1.35), Color("#4d5359"))
	_add_box(st, Vector3(x, y + 0.78, z), Vector3(0.42, 0.28, 0.42), Color("#6a9b91"))

func _append_dungeon_gate(st: SurfaceTool, anchor: Dictionary) -> void:
	var x := float(anchor.x)
	var y := float(anchor.y)
	var z := float(anchor.z)
	var dark_stone := Color("#494852")
	var edge_stone := Color("#77717c")
	# Heavy stepped threshold, paired rune pillars and a broken lintel.
	for step in range(3):
		_add_box(st, Vector3(x, y + 0.16 + float(step) * 0.16, z + 2.0 - float(step) * 0.55), Vector3(6.8 - float(step) * 0.65, 0.32, 1.15), dark_stone.lightened(float(step) * 0.035))
	for side in [-1.0, 1.0]:
		_add_box(st, Vector3(x + side * 3.0, y + 2.55, z), Vector3(1.0, 5.1, 1.15), dark_stone)
		_add_box(st, Vector3(x + side * 3.0, y + 4.95, z), Vector3(1.35, 0.52, 1.45), edge_stone)
		_add_box(st, Vector3(x + side * 3.0, y + 2.25, z - 0.63), Vector3(0.18, 2.1, 0.12), Color("#6f9b9b"))
	_add_box(st, Vector3(x, y + 5.0, z), Vector3(5.9, 0.9, 1.25), dark_stone)
	_add_box(st, Vector3(x, y + 5.5, z), Vector3(2.2, 0.22, 1.4), edge_stone)
	# A recessed, non-interactive void gives the entrance a dark-fantasy focal point.
	_add_box(st, Vector3(x, y + 2.45, z + 0.16), Vector3(4.25, 3.9, 0.16), Color("#211d31"))
	_add_box(st, Vector3(x, y + 2.45, z + 0.05), Vector3(0.12, 3.55, 0.12), Color("#4c777f"))
	_add_box(st, Vector3(x, y + 0.42, z - 0.12), Vector3(2.0, 0.32, 0.32), Color("#6b8b87"))

func _build_chunk(key: Vector2i) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var decoration_st := SurfaceTool.new()
	decoration_st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var start_x := key.x * CHUNK_SIZE
	var start_z := key.y * CHUNK_SIZE

	# Sample the chunk plus a one-cell halo once. The old path sampled each
	# center height/type and four neighbors independently (up to 7x noise work).
	var sample_grid: Array = []
	for local_z in range(-1, CHUNK_SIZE + 1):
		var row: Array = []
		for local_x in range(-1, CHUNK_SIZE + 1):
			row.append(_sample_world(start_x + local_x, start_z + local_z))
		sample_grid.append(row)

	for local_z in range(CHUNK_SIZE):
		for local_x in range(CHUNK_SIZE):
			var x := start_x + local_x
			var z := start_z + local_z
			var sample: Array = sample_grid[local_z + 1][local_x + 1]
			var y := int(sample[0])
			var block_type := int(sample[1])
			var color := _biome_color(int(sample[2]), block_type)
			var x0 := float(x) - 0.5
			var x1 := float(x) + 0.5
			var z0 := float(z) - 0.5
			var z1 := float(z) + 0.5
			var top := float(y) + 0.5
			_add_quad(st, Vector3(x0, top, z0), Vector3(x0, top, z1), Vector3(x1, top, z1), Vector3(x1, top, z0), color)
			var west: Array = sample_grid[local_z + 1][local_x]
			var east: Array = sample_grid[local_z + 1][local_x + 2]
			var north: Array = sample_grid[local_z][local_x + 1]
			var south: Array = sample_grid[local_z + 2][local_x + 1]
			var west_height := int(west[0])
			var east_height := int(east[0])
			var north_height := int(north[0])
			var south_height := int(south[0])
			if west_height < y:
				_add_quad(st, Vector3(x0, float(west_height) + 0.5, z0), Vector3(x0, float(west_height) + 0.5, z1), Vector3(x0, top, z1), Vector3(x0, top, z0), color.darkened(0.18))
			if east_height < y:
				_add_quad(st, Vector3(x1, float(east_height) + 0.5, z1), Vector3(x1, float(east_height) + 0.5, z0), Vector3(x1, top, z0), Vector3(x1, top, z1), color.darkened(0.18))
			if north_height < y:
				_add_quad(st, Vector3(x1, float(north_height) + 0.5, z0), Vector3(x0, float(north_height) + 0.5, z0), Vector3(x0, top, z0), Vector3(x1, top, z0), color.darkened(0.28))
			if south_height < y:
				_add_quad(st, Vector3(x0, float(south_height) + 0.5, z1), Vector3(x1, float(south_height) + 0.5, z1), Vector3(x1, top, z1), Vector3(x0, top, z1), color.darkened(0.28))
	var mesh := st.commit()
	if mesh == null or mesh.get_surface_count() == 0:
		return
	for anchor in decoration_anchors(key):
		match int(anchor.kind):
			5: _append_monolith(decoration_st, anchor)
			6: _append_ruin(decoration_st, anchor)
			7: _append_dungeon_gate(decoration_st, anchor)
			_: _append_tree(decoration_st, anchor)
	var decoration_mesh := decoration_st.commit()
	var root: Node3D
	if not chunk_pool.is_empty():
		root = chunk_pool.pop_back()
	else:
		root = Node3D.new()
		add_child(root)
	root.name = "Chunk_%d_%d" % [key.x, key.y]
	root.set_meta("chunk_coord", key)
	root.visible = true
	var visual := root.get_node_or_null("Visual") as MeshInstance3D
	if visual == null:
		visual = MeshInstance3D.new()
		visual.name = "Visual"
		root.add_child(visual)
	visual.mesh = mesh
	visual.material_override = material
	var decorations := root.get_node_or_null("Decorations") as MeshInstance3D
	if decoration_mesh != null and decoration_mesh.get_surface_count() > 0:
		if decorations == null:
			decorations = MeshInstance3D.new()
			decorations.name = "Decorations"
			root.add_child(decorations)
		decorations.mesh = decoration_mesh
		decorations.material_override = material
		decorations.visible = true
	elif decorations != null:
		# Recycled chunks may still own an old decoration mesh; hide it explicitly.
		decorations.visible = false
	var body := root.get_node_or_null("CollisionBody") as StaticBody3D
	if body == null:
		body = StaticBody3D.new()
		body.name = "CollisionBody"
		root.add_child(body)
	body.set_meta("chunk", key)
	body.collision_layer = 1
	body.collision_mask = 1
	var collision := body.get_node_or_null("CollisionShape3D") as CollisionShape3D
	if collision == null:
		collision = CollisionShape3D.new()
		collision.name = "CollisionShape3D"
		body.add_child(collision)
	collision.shape = mesh.create_trimesh_shape()
	chunks[key] = root

func cell_from_hit(hit: Dictionary) -> Vector3i:
	var p: Vector3 = hit.position
	var normal: Vector3 = hit.normal
	var x := roundi(p.x - normal.x * 0.015)
	var z := roundi(p.z - normal.z * 0.015)
	return Vector3i(x, _height(x, z), z)

func mine_surface(cell: Vector3i) -> int:
	var old_type := _block_type(cell.x, cell.z)
	var old_height := _height(cell.x, cell.z)
	if old_height <= 0:
		return -1
	terrain_edits["%d,%d" % [cell.x, cell.z]] = {"height": old_height - 1, "type": old_type}
	_rebuild_cell_and_seams(cell)
	return old_type

func _recycle_chunk(node: Node3D) -> void:
	node.visible = false
	var body := node.get_node_or_null("CollisionBody") as StaticBody3D
	if body:
		body.collision_layer = 0
		body.collision_mask = 0
	if chunk_pool.size() < MAX_POOLED_CHUNKS:
		chunk_pool.append(node)
	else:
		node.queue_free()

func _rebuild_chunk(key: Vector2i) -> void:
	if chunks.has(key):
		var old: Node3D = chunks[key]
		chunks.erase(key)
		_recycle_chunk(old)
	_build_chunk(key)

func _rebuild_cell_and_seams(cell: Vector3i) -> void:
	var key := _chunk_of_world(Vector3(cell.x, 0, cell.z))
	var rebuild: Array[Vector2i] = [key]
	var local_x := posmod(cell.x, CHUNK_SIZE)
	var local_z := posmod(cell.z, CHUNK_SIZE)
	if local_x == 0: rebuild.append(key + Vector2i(-1, 0))
	elif local_x == CHUNK_SIZE - 1: rebuild.append(key + Vector2i(1, 0))
	if local_z == 0: rebuild.append(key + Vector2i(0, -1))
	elif local_z == CHUNK_SIZE - 1: rebuild.append(key + Vector2i(0, 1))
	for adjacent in rebuild:
		if chunks.has(adjacent):
			_rebuild_chunk(adjacent)

func export_edits() -> Array:
	var result: Array = []
	for key in terrain_edits.keys():
		var parts: PackedStringArray = str(key).split(",")
		result.append([int(parts[0]), int(parts[1]), int(terrain_edits[key].height), int(terrain_edits[key].type)])
	return result

func import_edits(entries: Array) -> void:
	terrain_edits.clear()
	for entry in entries:
		if entry is Array and entry.size() == 4:
			terrain_edits["%d,%d" % [int(entry[0]), int(entry[1])]] = {"height": int(entry[2]), "type": int(entry[3])}
	for key in chunks.keys():
		_rebuild_chunk(key)

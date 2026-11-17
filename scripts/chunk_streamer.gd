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

func _build_chunk(key: Vector2i) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var start_x := key.x * CHUNK_SIZE
	var start_z := key.y * CHUNK_SIZE

	# Sample the chunk plus a one-cell halo once. The old path sampled each
	# center cell and its four neighbors independently (up to 5x noise work).
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
		var parts := key.split(",")
		result.append([int(parts[0]), int(parts[1]), int(terrain_edits[key].height), int(terrain_edits[key].type)])
	return result

func import_edits(entries: Array) -> void:
	terrain_edits.clear()
	for entry in entries:
		if entry is Array and entry.size() == 4:
			terrain_edits["%d,%d" % [int(entry[0]), int(entry[1])]] = {"height": int(entry[2]), "type": int(entry[3])}
	for key in chunks.keys():
		_rebuild_chunk(key)

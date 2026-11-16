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
	for z in range(center.y - LOAD_RADIUS, center.y + LOAD_RADIUS + 1):
		for x in range(center.x - LOAD_RADIUS, center.x + LOAD_RADIUS + 1):
			var key := Vector2i(x, z)
			wanted[key] = true
			if not chunks.has(key):
				_build_chunk(key)
	for key in chunks.keys():
		if not wanted.has(key) and maxi(abs(key.x - center.x), abs(key.y - center.y)) > UNLOAD_RADIUS:
			var node: Node3D = chunks[key]
			chunks.erase(key)
			node.queue_free()

func _chunk_of_world(position: Vector3) -> Vector2i:
	return Vector2i(floori(position.x / CHUNK_SIZE), floori(position.z / CHUNK_SIZE))

func _height(x: int, z: int) -> int:
	var key := "%d,%d" % [x, z]
	if terrain_edits.has(key):
		return int(terrain_edits[key].height)
	return int(generator.surface_at(x, z)[0])

func _block_type(x: int, z: int) -> int:
	var key := "%d,%d" % [x, z]
	if terrain_edits.has(key):
		return int(terrain_edits[key].type)
	return int(generator.surface_at(x, z)[1])

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
	for z in range(start_z, start_z + CHUNK_SIZE):
		for x in range(start_x, start_x + CHUNK_SIZE):
			var sample: Array = generator.surface_at(x, z)
			var y := _height(x, z)
			var block_type := _block_type(x, z)
			var color := _biome_color(int(sample[2]), block_type)
			var x0 := float(x) - 0.5
			var x1 := float(x) + 0.5
			var z0 := float(z) - 0.5
			var z1 := float(z) + 0.5
			var top := float(y) + 0.5
			_add_quad(st, Vector3(x0, top, z0), Vector3(x1, top, z0), Vector3(x1, top, z1), Vector3(x0, top, z1), color)
			var west := _height(x - 1, z)
			var east := _height(x + 1, z)
			var north := _height(x, z - 1)
			var south := _height(x, z + 1)
			if west < y:
				_add_quad(st, Vector3(x0, float(west) + 0.5, z1), Vector3(x0, float(west) + 0.5, z0), Vector3(x0, top, z0), Vector3(x0, top, z1), color.darkened(0.18))
			if east < y:
				_add_quad(st, Vector3(x1, float(east) + 0.5, z0), Vector3(x1, float(east) + 0.5, z1), Vector3(x1, top, z1), Vector3(x1, top, z0), color.darkened(0.18))
			if north < y:
				_add_quad(st, Vector3(x0, float(north) + 0.5, z0), Vector3(x1, float(north) + 0.5, z0), Vector3(x1, top, z0), Vector3(x0, top, z0), color.darkened(0.28))
			if south < y:
				_add_quad(st, Vector3(x1, float(south) + 0.5, z1), Vector3(x0, float(south) + 0.5, z1), Vector3(x0, top, z1), Vector3(x1, top, z1), color.darkened(0.28))
	var mesh := st.commit()
	if mesh == null or mesh.get_surface_count() == 0:
		return
	var root := Node3D.new()
	root.name = "Chunk_%d_%d" % [key.x, key.y]
	root.set_meta("chunk_coord", key)
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	visual.material_override = material
	root.add_child(visual)
	var body := StaticBody3D.new()
	body.set_meta("chunk", key)
	body.collision_layer = 1
	body.collision_mask = 1
	var collision := CollisionShape3D.new()
	collision.shape = mesh.create_trimesh_shape()
	body.add_child(collision)
	root.add_child(body)
	add_child(root)
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
	_rebuild_chunk(_chunk_of_world(Vector3(cell.x, 0, cell.z)))
	return old_type

func _rebuild_chunk(key: Vector2i) -> void:
	if chunks.has(key):
		var old: Node3D = chunks[key]
		chunks.erase(key)
		old.queue_free()
	_build_chunk(key)

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

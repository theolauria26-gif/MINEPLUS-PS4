extends RefCounted
class_name MineplusWorldGen
## Geração determinística por coordenadas globais. Não depende da ordem de carregamento dos chunks.

enum Biome { PLAINS, FOREST, DESERT, TUNDRA, VOLCANIC, SWAMP, HIGHLANDS, OCEAN }
enum Block { GRASS, DIRT, STONE, SAND, MOSS, BRICK, SNOW, ICE, BASALT, WATER, WOOD }

const CENTRAL_RADIUS := 420.0
const OCEAN_WIDTH := 76.0
const SEA_LEVEL := 7
const STRUCTURE_CELL := 64

var world_seed: int
var terrain_noise := FastNoiseLite.new()
var temperature_noise := FastNoiseLite.new()
var humidity_noise := FastNoiseLite.new()

func _init(seed_value: int = 4839201) -> void:
	world_seed = seed_value
	terrain_noise.seed = world_seed
	terrain_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	terrain_noise.fractal_type = FastNoiseLite.FRACTAL_FBM
	terrain_noise.fractal_octaves = 5
	terrain_noise.frequency = 0.009
	terrain_noise.fractal_lacunarity = 2.05
	terrain_noise.fractal_gain = 0.48
	temperature_noise.seed = world_seed ^ 0x13579
	temperature_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	temperature_noise.frequency = 0.0018
	humidity_noise.seed = world_seed ^ 0x24680
	humidity_noise.noise_type = FastNoiseLite.TYPE_SIMPLEX
	humidity_noise.frequency = 0.0022

static func _hash2(x: int, z: int, seed_value: int) -> int:
	var n: int = x * 374761393 + z * 668265263 + seed_value * 1442695041
	n = (n ^ (n >> 13)) * 1274126177
	return int(n ^ (n >> 16)) & 0x7fffffff

func biome_at(x: int, z: int) -> int:
	var distance := Vector2(float(x), float(z)).length()
	if distance > CENTRAL_RADIUS and distance < CENTRAL_RADIUS + OCEAN_WIDTH:
		return Biome.OCEAN

	var temperature := temperature_noise.get_noise_2d(float(x), float(z))
	var humidity := humidity_noise.get_noise_2d(float(x), float(z))
	if distance >= CENTRAL_RADIUS + OCEAN_WIDTH:
		# Terras infinitas: macro-regiões determinísticas, sem repetir um mapa finito.
		var macro_x := floori(float(x) / 256.0)
		var macro_z := floori(float(z) / 256.0)
		var roll := _hash2(macro_x, macro_z, world_seed) % 100
		if roll < 17: return Biome.DESERT
		if roll < 34: return Biome.TUNDRA
		if roll < 48: return Biome.VOLCANIC
		if roll < 65: return Biome.HIGHLANDS
		if roll < 82: return Biome.SWAMP
		if humidity > 0.12: return Biome.FOREST
		return Biome.PLAINS

	# Continente central: zonas climáticas coerentes e transições via mapas de clima.
	if temperature < -0.34: return Biome.TUNDRA
	if temperature > 0.38 and humidity < -0.12: return Biome.DESERT
	if humidity > 0.38: return Biome.SWAMP
	if temperature > 0.28 and humidity > 0.08: return Biome.FOREST
	if terrain_noise.get_noise_2d(float(x) * 0.42, float(z) * 0.42) > 0.42:
		return Biome.HIGHLANDS
	return Biome.PLAINS

func ground_height_at(x: int, z: int) -> int:
	var distance := Vector2(float(x), float(z)).length()
	if distance > CENTRAL_RADIUS and distance < CENTRAL_RADIUS + OCEAN_WIDTH:
		return 1 + roundi(terrain_noise.get_noise_2d(float(x), float(z)) * 2.0)
	var biome := biome_at(x, z)
	var continental := terrain_noise.get_noise_2d(float(x), float(z))
	var detail := terrain_noise.get_noise_2d(float(x) * 2.7, float(z) * 2.7) * 2.0
	var amplitude := 7.0
	if biome == Biome.HIGHLANDS: amplitude = 19.0
	elif biome == Biome.VOLCANIC: amplitude = 12.0
	elif biome == Biome.DESERT: amplitude = 4.0
	elif biome == Biome.SWAMP: amplitude = 2.5
	return 9 + roundi(continental * amplitude + detail)

func _structure_anchor(gx: int, gz: int) -> Dictionary:
	var h := _hash2(gx, gz, world_seed ^ 0x5f3759df)
	var chance := h % 100
	if chance < 9:
		return {"type": 1, "x": gx * STRUCTURE_CELL + 12 + (h % 39), "z": gz * STRUCTURE_CELL + 12 + ((_hash2(gz, gx, world_seed) % 39))}
	if chance < 13:
		return {"type": 2, "x": gx * STRUCTURE_CELL + 16 + (h % 31), "z": gz * STRUCTURE_CELL + 16 + ((_hash2(gz, gx, world_seed) % 31))}
	return {}

## Retorna [altura da superfície, tipo de bloco, bioma].
## A busca por âncoras vizinhas permite árvores/ruínas atravessarem limites de chunk.
func surface_at(x: int, z: int) -> Array:
	var biome := biome_at(x, z)
	var height := ground_height_at(x, z)
	var block_type := Block.GRASS
	match biome:
		Biome.FOREST: block_type = Block.MOSS
		Biome.DESERT: block_type = Block.SAND
		Biome.TUNDRA: block_type = Block.SNOW
		Biome.VOLCANIC: block_type = Block.BASALT
		Biome.SWAMP: block_type = Block.MOSS
		Biome.HIGHLANDS: block_type = Block.STONE
		Biome.OCEAN:
			return [SEA_LEVEL, Block.WATER, biome]

	var gx := floori(float(x) / STRUCTURE_CELL)
	var gz := floori(float(z) / STRUCTURE_CELL)
	for oz in range(-1, 2):
		for ox in range(-1, 2):
			var anchor := _structure_anchor(gx + ox, gz + oz)
			if anchor.is_empty():
				continue
			var ax: int = anchor.x
			var az: int = anchor.z
			var dx := x - ax
			var dz := z - az
			var dist_sq := dx * dx + dz * dz
			if anchor.type == 1:
				# Árvore gigante: tronco alto e copa em camadas.
				if dx == 0 and dz == 0:
					height += 9
					block_type = Block.WOOD
				elif dist_sq <= 12 and abs(dx) <= 3 and abs(dz) <= 3:
					height += 7 - int(dist_sq / 5)
					block_type = Block.MOSS
			elif anchor.type == 2:
				# Ruína circular: quatro pilares e paredes baixas.
				if dist_sq >= 25 and dist_sq <= 36:
					if abs(dx) == 5 or abs(dz) == 5:
						height += 3
						block_type = Block.BRICK
				elif dist_sq <= 2:
					height += 1
					block_type = Block.BRICK
	return [height, block_type, biome]

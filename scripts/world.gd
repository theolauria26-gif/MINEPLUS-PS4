extends Node3D

const WORLD_RADIUS := 9
const BLOCK_SIZE := 1.0
const MAX_STACK_SIZE := 999
const BLOCK_COLORS := [
	Color("#79b85a"), Color("#8b6548"), Color("#858b96"), Color("#c6ad72"), Color("#4e9d83"), Color("#b86f62"), Color("#9a704a")
]
const BLOCK_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo", "Tábuas"]
const INVENTORY_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo", "Madeira", "Tábuas"]
const INVENTORY_COLORS := [Color("#79b85a"), Color("#8b6548"), Color("#858b96"), Color("#c6ad72"), Color("#4e9d83"), Color("#b86f62"), Color("#b88754"), Color("#d0a16b")]

var blocks: Dictionary = {}
var chunk_streamer: MineplusChunkStreamer
var selected_block := 0
var block_counts: Array[int] = [64, 64, 32, 32, 16, 0, 0, 0]
var player: CharacterBody3D
var camera: Camera3D
var pickaxe_visual: Node3D
var hotbar_label: Label
var hotbar_buttons: Array[Button] = []
var status_label: Label
var pitch := 0.0
var touch_look_active := false
var touch_look_index := -1
var inventory_panel: PanelContainer
var inventory_grid: GridContainer
var inventory_panel_width := 460.0
var pause_layer: CanvasLayer
var inventory_tool_label: Label
var inventory_hand_button: Button
var inventory_pickaxe_button: Button
var inventory_axe_button: Button
var inventory_filter := MineplusInventory.CATEGORY_ALL
var inventory_filter_buttons: Array[Button] = []
var look_sensitivity := 0.0025
var configured_fov := 78.0
var hud_opacity := 1.0
var graphics_quality := 1
var pickaxe_durability := 0
var axe_durability := 0
var selected_tool := MineplusEquipment.TOOL_NONE
var axe_visual: Node3D
var block_textures: Array[Texture2D] = []
var first_person_arms: Array[MeshInstance3D] = []
var walk_animation_time := 0.0
var camera_base_height := 1.55
var world_slot := 1
var autosave_elapsed := 0.0
const AUTOSAVE_INTERVAL := 45.0
var selected_skin := 0
var saved_player_position := Vector3(0, 13.0, 5)
var saved_player_rotation_y := 0.0
var saved_camera_pitch := 0.0
var saved_selected_block := 0
var world_environment: Environment
var sky_material: ProceduralSkyMaterial
var sun_light: DirectionalLight3D
var day_clock := 0.5
const DAY_LENGTH_SECONDS := 900.0
const SKIN_COLORS := [Color("#d9a47c"), Color("#8b5a3c"), Color("#e7c7a5"), Color("#c8b0e8")]
const SKIN_SLEEVE_COLORS := [Color("#526f58"), Color("#744c45"), Color("#536d83"), Color("#66527e")]

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_load_settings()
	_setup_environment()
	_setup_block_textures()
	_generate_world()
	_load_world()
	_create_player()
	chunk_streamer.player = player
	_create_hud()
	if not OS.has_feature("mobile"):
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _setup_environment() -> void:
	var world_env := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = Sky.new()
	sky_material = ProceduralSkyMaterial.new()
	sky_material.sky_top_color = Color("#182238")
	sky_material.sky_horizon_color = Color("#705e69")
	sky_material.ground_bottom_color = Color("#151b20")
	sky_material.ground_horizon_color = Color("#3c4948")
	env.sky.sky_material = sky_material
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("#9aabb0")
	env.ambient_light_energy = 0.7 + float(graphics_quality) * 0.1
	# Atmospheric depth is enabled on the actual 3D scene, not painted into tiles.
	env.fog_enabled = true
	env.fog_light_color = Color("#667783")
	env.fog_light_energy = 0.8
	env.fog_density = 0.006
	env.fog_sky_affect = 0.28
	env.fog_aerial_perspective = 0.18
	env.glow_enabled = graphics_quality >= 2
	env.glow_intensity = 0.32
	world_environment = env
	world_env.environment = env
	add_child(world_env)
	sun_light = DirectionalLight3D.new()
	sun_light.rotation_degrees = Vector3(-72, -28, 0)
	sun_light.light_energy = 1.0 + float(graphics_quality) * 0.1
	sun_light.light_color = Color("#ffe0b4")
	sun_light.shadow_enabled = graphics_quality >= 2
	add_child(sun_light)
	_update_environment(0.0)

func _generate_world() -> void:
	# Mundo base infinito em chunks; construções do jogador ficam no save separado.
	# A seed é salva por mundo e todas as amostras usam coordenadas globais.
	chunk_streamer = MineplusChunkStreamer.new()
	chunk_streamer.generator = MineplusWorldGen.new(MineplusWorldLobby.default_world_seed(world_slot))
	chunk_streamer.configure_quality(graphics_quality, OS.has_feature("mobile"))
	chunk_streamer.name = "ChunkStreamer"
	add_child(chunk_streamer)


func _setup_block_textures() -> void:
	block_textures.clear()
	for block_type in range(BLOCK_COLORS.size()):
		var image := Image.create(16, 16, false, Image.FORMAT_RGBA8)
		var base: Color = BLOCK_COLORS[block_type]
		for y in range(16):
			for x in range(16):
				var variation := ((x * 13 + y * 7 + block_type * 11) % 9) - 4
				var shade := 1.0 + float(variation) * 0.035
				var pixel := Color(
					clampf(base.r * shade, 0.0, 1.0),
					clampf(base.g * shade, 0.0, 1.0),
					clampf(base.b * shade, 0.0, 1.0),
					1.0
				)
				if block_type == 0 and y < 4:
					pixel = Color("#8bc96b") if (x + y) % 3 != 0 else Color("#6da84e")
				elif block_type == 4 and (x * 3 + y * 5) % 11 == 0:
					pixel = Color("#367c68")
				elif block_type == 5 and (x % 8 == 0 or y % 8 == 0):
					pixel = Color("#8f514b")
				elif block_type == 6:
					if y % 8 == 0:
						pixel = Color("#67482f")
					elif (x + int(y / 8) * 3) % 13 == 0:
						pixel = Color("#d0a16b")
				image.set_pixel(x, y, pixel)
		block_textures.append(ImageTexture.create_from_image(image))

func _add_block(cell: Vector3i, block_type: int) -> void:
	if blocks.has(cell):
		return
	var body := StaticBody3D.new()
	body.position = Vector3(cell) * BLOCK_SIZE
	body.set_meta("cell", cell)
	body.set_meta("block_type", block_type)
	var mesh := BoxMesh.new()
	mesh.size = Vector3.ONE * BLOCK_SIZE
	var visual := MeshInstance3D.new()
	visual.mesh = mesh
	var material := StandardMaterial3D.new()
	material.albedo_color = Color.WHITE
	material.albedo_texture = block_textures[block_type]
	material.roughness = 0.92
	visual.material_override = material
	body.add_child(visual)
	var collision := CollisionShape3D.new()
	var shape := BoxShape3D.new()
	shape.size = Vector3.ONE * BLOCK_SIZE
	collision.shape = shape
	body.add_child(collision)
	add_child(body)
	blocks[cell] = body

func _remove_block(cell: Vector3i) -> void:
	if not blocks.has(cell):
		return
	var body: StaticBody3D = blocks[cell]
	blocks.erase(cell)
	body.queue_free()

func _create_player() -> void:
	# Saves antigos vinham de um terreno plano em Y=0; eleva o spawn se ficar soterrado.
	var floor_height := int(chunk_streamer.generator.surface_at(roundi(saved_player_position.x), roundi(saved_player_position.z))[0])
	if saved_player_position.y < float(floor_height) + 2.2:
		saved_player_position.y = float(floor_height) + 2.2
	player = CharacterBody3D.new()
	player.position = saved_player_position
	player.rotation.y = saved_player_rotation_y
	player.name = "Player"
	var capsule := CollisionShape3D.new()
	var shape := CapsuleShape3D.new()
	shape.radius = 0.32
	shape.height = 1.8
	capsule.shape = shape
	capsule.position.y = 0.9
	player.add_child(capsule)
	camera = Camera3D.new()
	camera.position.y = camera_base_height
	camera.current = true
	camera.fov = configured_fov
	camera.rotation.x = saved_camera_pitch
	pitch = saved_camera_pitch
	player.add_child(camera)
	_create_first_person_arms()
	_create_pickaxe_visual()
	_create_axe_visual()
	add_child(player)
	selected_block = clampi(saved_selected_block, 0, BLOCK_NAMES.size() - 1)

func _create_first_person_arms() -> void:
	first_person_arms.clear()
	var arm_material := StandardMaterial3D.new()
	arm_material.albedo_color = SKIN_COLORS[selected_skin]
	arm_material.roughness = 0.85
	var sleeve_material := StandardMaterial3D.new()
	sleeve_material.albedo_color = SKIN_SLEEVE_COLORS[selected_skin]
	sleeve_material.roughness = 0.9
	for side in [-1.0, 1.0]:
		var arm := MeshInstance3D.new()
		var arm_mesh := BoxMesh.new()
		arm_mesh.size = Vector3(0.16, 0.42, 0.16)
		arm.mesh = arm_mesh
		arm.material_override = arm_material
		arm.position = Vector3(side * 0.34, -0.43, -0.52)
		arm.rotation_degrees.z = side * -12.0
		arm.set_meta("side", side)
		camera.add_child(arm)
		var cuff := MeshInstance3D.new()
		var cuff_mesh := BoxMesh.new()
		cuff_mesh.size = Vector3(0.174, 0.085, 0.174)
		cuff.mesh = cuff_mesh
		cuff.material_override = sleeve_material
		cuff.position = Vector3(0.0, -0.15, 0.0)
		arm.add_child(cuff)
		first_person_arms.append(arm)

static func _walk_arm_offset(phase: float, side: float, movement_amount: float) -> Vector3:
	var intensity := clampf(movement_amount, 0.0, 1.0)
	var wave := phase + side * PI * 0.5
	return Vector3(0.0, sin(wave) * 0.035 * intensity, cos(wave) * 0.025 * intensity)

static func _walk_arm_rotation(phase: float, side: float, movement_amount: float) -> Vector3:
	var intensity := clampf(movement_amount, 0.0, 1.0)
	var wave := phase + side * PI * 0.5
	return Vector3(sin(wave) * 3.5 * intensity, 0.0, cos(wave) * 4.0 * intensity)

func _update_first_person_animation(movement_amount: float) -> void:
	for arm in first_person_arms:
		var side := float(arm.get_meta("side", 1.0))
		arm.position = Vector3(side * 0.34, -0.43, -0.52) + _walk_arm_offset(walk_animation_time, side, movement_amount)
		var swing := _walk_arm_rotation(walk_animation_time, side, movement_amount)
		arm.rotation_degrees = Vector3(swing.x, 0.0, side * -12.0 + swing.z)

func _create_pickaxe_visual() -> void:
	pickaxe_visual = Node3D.new()
	pickaxe_visual.position = Vector3(0.38, -0.36, -0.62)
	pickaxe_visual.rotation_degrees = Vector3(-18, -24, -18)
	pickaxe_visual.visible = selected_tool == MineplusEquipment.TOOL_PICKAXE and pickaxe_durability > 0
	camera.add_child(pickaxe_visual)
	var handle := MeshInstance3D.new()
	var handle_mesh := BoxMesh.new()
	handle_mesh.size = Vector3(0.075, 0.48, 0.075)
	handle.mesh = handle_mesh
	var wood := StandardMaterial3D.new()
	wood.albedo_color = Color("#815331")
	handle.material_override = wood
	handle.position.y = -0.06
	pickaxe_visual.add_child(handle)
	var head := MeshInstance3D.new()
	var head_mesh := BoxMesh.new()
	head_mesh.size = Vector3(0.34, 0.12, 0.12)
	head.mesh = head_mesh
	var metal := StandardMaterial3D.new()
	metal.albedo_color = Color("#aeb9c4")
	metal.metallic = 0.72
	metal.roughness = 0.3
	head.material_override = metal
	head.position.y = 0.16
	pickaxe_visual.add_child(head)

func _animate_pickaxe_swing() -> void:
	if pickaxe_visual == null or not pickaxe_visual.visible:
		return
	var tween := create_tween()
	tween.tween_property(pickaxe_visual, "rotation_degrees", Vector3(-62, -4, -30), 0.08)
	tween.tween_property(pickaxe_visual, "rotation_degrees", Vector3(-18, -24, -18), 0.12)

func _create_axe_visual() -> void:
	axe_visual = Node3D.new()
	axe_visual.position = Vector3(0.34, -0.37, -0.60)
	axe_visual.rotation_degrees = Vector3(-18, -18, -28)
	axe_visual.visible = selected_tool == MineplusEquipment.TOOL_AXE and axe_durability > 0
	camera.add_child(axe_visual)
	var handle := MeshInstance3D.new()
	var handle_mesh := BoxMesh.new()
	handle_mesh.size = Vector3(0.075, 0.46, 0.075)
	handle.mesh = handle_mesh
	var wood := StandardMaterial3D.new()
	wood.albedo_color = Color("#815331")
	handle.material_override = wood
	handle.position.y = -0.04
	axe_visual.add_child(handle)
	var head := MeshInstance3D.new()
	var head_mesh := BoxMesh.new()
	head_mesh.size = Vector3(0.28, 0.16, 0.12)
	head.mesh = head_mesh
	var stone := StandardMaterial3D.new()
	stone.albedo_color = Color("#89949b")
	stone.roughness = 0.96
	head.material_override = stone
	head.position = Vector3(0.11, 0.15, 0.0)
	axe_visual.add_child(head)

func _animate_axe_swing() -> void:
	if axe_visual == null or not axe_visual.visible:
		return
	var tween := create_tween()
	tween.tween_property(axe_visual, "rotation_degrees", Vector3(-62, 8, -46), 0.09)
	tween.tween_property(axe_visual, "rotation_degrees", Vector3(-18, -18, -28), 0.13)

func _create_hud() -> void:
	var hud := CanvasLayer.new()
	add_child(hud)
	var hud_root := Control.new()
	hud_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	hud_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	hud_root.modulate.a = hud_opacity
	hud.add_child(hud_root)
	var top := Label.new()
	top.text = "MINEPLUS  /  SURVIVAL PROTOTYPE"
	top.position = Vector2(22, 18)
	top.add_theme_color_override("font_color", Color("#e9f2f2"))
	top.add_theme_font_size_override("font_size", 15)
	hud_root.add_child(top)
	status_label = Label.new()
	status_label.text = "WASD mover  •  Mouse olhar  •  Clique quebrar/colocar"
	status_label.position = Vector2(22, 44)
	status_label.add_theme_color_override("font_color", Color("#c2d0d0"))
	status_label.add_theme_font_size_override("font_size", 12)
	hud_root.add_child(status_label)
	var crosshair := Label.new()
	crosshair.text = "+"
	crosshair.set_anchors_preset(Control.PRESET_CENTER)
	crosshair.position = Vector2(-7, -15)
	crosshair.add_theme_font_size_override("font_size", 26)
	crosshair.add_theme_color_override("font_color", Color("#ffffff"))
	hud_root.add_child(crosshair)
	hotbar_label = Label.new()
	hotbar_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	var hud_width := minf(500.0, get_viewport().get_visible_rect().size.x - 20.0)
	hotbar_label.position = Vector2(-hud_width * 0.5, -74)
	hotbar_label.size = Vector2(hud_width, 44)
	hotbar_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hotbar_label.add_theme_font_size_override("font_size", 17)
	hotbar_label.add_theme_color_override("font_color", Color("#f1f5f2"))
	hud_root.add_child(hotbar_label)
	_update_hotbar()
	if OS.has_feature("mobile"):
		_add_touch_controls(hud_root)
	_add_hotbar_buttons(hud_root)
	_add_save_button(hud_root)
	_add_craft_button(hud_root)
	_add_inventory_button(hud_root)
	_create_inventory_panel(hud_root)
	_create_pause_menu(hud_root)

func _add_touch_controls(hud: Control) -> void:
	var viewport_size := get_viewport().get_visible_rect().size
	var ui_scale := clampf(minf(viewport_size.x / 720.0, viewport_size.y / 560.0), 0.55, 1.0)
	var move_box := HBoxContainer.new()
	move_box.position = Vector2(14, -112 * ui_scale)
	move_box.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	move_box.add_theme_constant_override("separation", maxi(3, roundi(7.0 * ui_scale)))
	hud.add_child(move_box)
	_add_hold_button(move_box, "◀", "move_left", ui_scale)
	_add_hold_button(move_box, "▲", "move_forward", ui_scale)
	_add_hold_button(move_box, "▼", "move_back", ui_scale)
	_add_hold_button(move_box, "▶", "move_right", ui_scale)
	var break_button := Button.new()
	break_button.text = "QUEBRAR"
	break_button.position = Vector2(-218 * ui_scale, -112 * ui_scale)
	break_button.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	break_button.custom_minimum_size = Vector2(96, 54) * ui_scale
	break_button.add_theme_font_size_override("font_size", 10)
	break_button.pressed.connect(func(): _interact(false))
	hud.add_child(break_button)
	var place_button := Button.new()
	place_button.text = "COLOCAR"
	place_button.position = Vector2(-322 * ui_scale, -112 * ui_scale)
	place_button.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	place_button.custom_minimum_size = Vector2(96, 54) * ui_scale
	place_button.add_theme_font_size_override("font_size", 10)
	place_button.pressed.connect(func(): _interact(true))
	hud.add_child(place_button)
	var jump := Button.new()
	jump.text = "PULAR"
	jump.position = Vector2(-112 * ui_scale, -112 * ui_scale)
	jump.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	jump.custom_minimum_size = Vector2(88, 54) * ui_scale
	jump.add_theme_font_size_override("font_size", 10)
	jump.button_down.connect(func(): Input.action_press("jump"))
	jump.button_up.connect(func(): Input.action_release("jump"))
	hud.add_child(jump)

func _add_hotbar_buttons(hud: Control) -> void:
	var bar := HBoxContainer.new()
	bar.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	var viewport_width := get_viewport().get_visible_rect().size.x
	var bar_width := minf(520.0, viewport_width - 16.0)
	bar.position = Vector2(-bar_width * 0.5, -126)
	bar.size = Vector2(bar_width, 42)
	bar.alignment = BoxContainer.ALIGNMENT_CENTER
	bar.add_theme_constant_override("separation", 4)
	hud.add_child(bar)
	for i in range(BLOCK_NAMES.size()):
		var slot := Button.new()
		slot.text = str(i + 1) + " " + BLOCK_NAMES[i].substr(0, 3).to_upper()
		slot.custom_minimum_size = Vector2((bar_width - 24.0) / float(BLOCK_NAMES.size()), 38)
		slot.add_theme_font_size_override("font_size", 9)
		slot.pressed.connect(_select_block.bind(i))
		bar.add_child(slot)
		hotbar_buttons.append(slot)
	_update_hotbar()

func _select_block(index: int) -> void:
	selected_block = clampi(index, 0, BLOCK_NAMES.size() - 1)
	_update_hotbar()

func _add_inventory_button(hud: Control) -> void:
	var button := Button.new()
	button.text = "INVENTÁRIO"
	button.set_anchors_preset(Control.PRESET_TOP_LEFT)
	button.position = Vector2(166, 76)
	button.custom_minimum_size = Vector2(132, 42)
	button.pressed.connect(_toggle_inventory)
	hud.add_child(button)

func _create_inventory_panel(hud: Control) -> void:
	inventory_panel = PanelContainer.new()
	inventory_panel.set_anchors_preset(Control.PRESET_CENTER)
	var viewport_size := get_viewport().get_visible_rect().size
	var panel_width := minf(460.0, viewport_size.x - 20.0)
	inventory_panel_width = panel_width
	var panel_height := minf(380.0, viewport_size.y - 40.0)
	inventory_panel.position = Vector2(-panel_width * 0.5, -panel_height * 0.5)
	inventory_panel.size = Vector2(panel_width, panel_height)
	inventory_panel.visible = false
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.035, 0.045, 0.06, 0.96)
	style.border_color = Color("#8ee3b1")
	style.set_border_width_all(1)
	style.set_corner_radius_all(16)
	style.content_margin_left = 18
	style.content_margin_right = 18
	style.content_margin_top = 16
	style.content_margin_bottom = 16
	inventory_panel.add_theme_stylebox_override("panel", style)
	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 12)
	inventory_panel.add_child(stack)
	var heading := Label.new()
	heading.text = "INVENTÁRIO"
	heading.add_theme_font_size_override("font_size", 22)
	heading.add_theme_color_override("font_color", Color("#8ee3b1"))
	stack.add_child(heading)
	var hint := Label.new()
	hint.text = "Materiais coletados, ferramentas e recursos disponíveis"
	hint.add_theme_font_size_override("font_size", 12)
	stack.add_child(hint)
	inventory_tool_label = Label.new()
	inventory_tool_label.text = "PICARETA %d/40  •  MACHADO %d/40" % [pickaxe_durability, axe_durability]
	inventory_tool_label.add_theme_color_override("font_color", Color("#e9d28b"))
	inventory_tool_label.add_theme_font_size_override("font_size", 10)
	inventory_tool_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	inventory_tool_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	stack.add_child(inventory_tool_label)
	var tool_row := HBoxContainer.new()
	tool_row.add_theme_constant_override("separation", 5)
	inventory_hand_button = Button.new()
	inventory_hand_button.text = "MÃO LIVRE"
	inventory_hand_button.add_theme_font_size_override("font_size", 10)
	inventory_hand_button.custom_minimum_size.y = 32
	inventory_hand_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_hand_button.pressed.connect(_set_selected_tool.bind(MineplusEquipment.TOOL_NONE))
	tool_row.add_child(inventory_hand_button)
	inventory_pickaxe_button = Button.new()
	inventory_pickaxe_button.add_theme_font_size_override("font_size", 10)
	inventory_pickaxe_button.custom_minimum_size.y = 32
	inventory_pickaxe_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_pickaxe_button.pressed.connect(_set_selected_tool.bind(MineplusEquipment.TOOL_PICKAXE))
	tool_row.add_child(inventory_pickaxe_button)
	inventory_axe_button = Button.new()
	inventory_axe_button.add_theme_font_size_override("font_size", 10)
	inventory_axe_button.custom_minimum_size.y = 32
	inventory_axe_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_axe_button.pressed.connect(_set_selected_tool.bind(MineplusEquipment.TOOL_AXE))
	tool_row.add_child(inventory_axe_button)
	stack.add_child(tool_row)
	var filter_row := HBoxContainer.new()
	filter_row.add_theme_constant_override("separation", 6)
	for category in [MineplusInventory.CATEGORY_ALL, MineplusInventory.CATEGORY_BLOCKS, MineplusInventory.CATEGORY_MATERIALS]:
		var filter_button := Button.new()
		filter_button.text = MineplusInventory.category_title(category)
		filter_button.custom_minimum_size = Vector2(0, 34)
		filter_button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		filter_button.add_theme_font_size_override("font_size", 10)
		filter_button.pressed.connect(_set_inventory_filter.bind(category))
		filter_row.add_child(filter_button)
		inventory_filter_buttons.append(filter_button)
	stack.add_child(filter_row)
	var inventory_scroll := ScrollContainer.new()
	inventory_scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	inventory_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	inventory_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_AUTO
	stack.add_child(inventory_scroll)
	inventory_grid = GridContainer.new()
	inventory_grid.columns = 2 if panel_width >= 420.0 else 1
	inventory_grid.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	inventory_grid.add_theme_constant_override("h_separation", 10)
	inventory_grid.add_theme_constant_override("v_separation", 8)
	inventory_scroll.add_child(inventory_grid)
	var close := Button.new()
	close.text = "FECHAR  [E]"
	close.pressed.connect(_toggle_inventory)
	stack.add_child(close)
	hud.add_child(inventory_panel)
	_refresh_inventory()

func _toggle_inventory() -> void:
	if inventory_panel == null:
		return
	touch_look_active = false
	touch_look_index = -1
	inventory_panel.visible = not inventory_panel.visible
	if inventory_panel.visible:
		_refresh_inventory()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	elif not OS.has_feature("mobile"):
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _set_inventory_filter(category: String) -> void:
	if category not in [MineplusInventory.CATEGORY_ALL, MineplusInventory.CATEGORY_BLOCKS, MineplusInventory.CATEGORY_MATERIALS]:
		return
	inventory_filter = category
	_refresh_inventory()

func _set_selected_tool(tool: String, announce: bool = true) -> void:
	if tool == MineplusEquipment.TOOL_PICKAXE and pickaxe_durability <= 0:
		if status_label:
			status_label.text = "Você não tem uma picareta utilizável. Fabrique uma na bancada."
		return
	if tool == MineplusEquipment.TOOL_AXE and axe_durability <= 0:
		if status_label:
			status_label.text = "Você não tem um machado utilizável. Fabrique um na bancada."
		return
	if tool not in [MineplusEquipment.TOOL_NONE, MineplusEquipment.TOOL_PICKAXE, MineplusEquipment.TOOL_AXE]:
		return
	selected_tool = tool
	_update_tool_visuals()
	_refresh_inventory()
	if announce and status_label:
		var label := "mão livre" if tool.is_empty() else ("picareta" if tool == MineplusEquipment.TOOL_PICKAXE else "machado")
		status_label.text = "Equipamento selecionado: %s." % label
	_save_world(true)

func _update_tool_visuals() -> void:
	if pickaxe_visual:
		pickaxe_visual.visible = selected_tool == MineplusEquipment.TOOL_PICKAXE and pickaxe_durability > 0
	if axe_visual:
		axe_visual.visible = selected_tool == MineplusEquipment.TOOL_AXE and axe_durability > 0

func _equip_inventory_item(item_index: int) -> void:
	var block_type := MineplusInventory.block_type_for_item(item_index)
	if block_type < 0:
		if status_label:
			status_label.text = "Madeira é matéria-prima. Fabrique tábuas para construir."
		return
	_select_block(block_type)
	if status_label:
		status_label.text = "Bloco equipado: %s." % BLOCK_NAMES[block_type]

func _refresh_inventory() -> void:
	if inventory_grid == null:
		return
	if inventory_tool_label:
		var equipped_name := "MÃO LIVRE" if selected_tool.is_empty() else ("PICARETA" if selected_tool == MineplusEquipment.TOOL_PICKAXE else "MACHADO")
		inventory_tool_label.text = "EQUIPADO: %s  •  PICARETA %d/40  •  MACHADO %d/40" % [equipped_name, pickaxe_durability, axe_durability]
	if inventory_hand_button:
		inventory_hand_button.modulate = Color("#8ee3b1") if selected_tool.is_empty() else Color.WHITE
	if inventory_pickaxe_button:
		inventory_pickaxe_button.text = "PICARETA %d/40" % pickaxe_durability
		inventory_pickaxe_button.disabled = pickaxe_durability <= 0
		inventory_pickaxe_button.modulate = Color("#8ee3b1") if selected_tool == MineplusEquipment.TOOL_PICKAXE else Color.WHITE
	if inventory_axe_button:
		inventory_axe_button.text = "MACHADO %d/40" % axe_durability
		inventory_axe_button.disabled = axe_durability <= 0
		inventory_axe_button.modulate = Color("#8ee3b1") if selected_tool == MineplusEquipment.TOOL_AXE else Color.WHITE
	for button_index in range(inventory_filter_buttons.size()):
		var filter_button := inventory_filter_buttons[button_index]
		var categories := [MineplusInventory.CATEGORY_ALL, MineplusInventory.CATEGORY_BLOCKS, MineplusInventory.CATEGORY_MATERIALS]
		filter_button.modulate = Color("#8ee3b1") if categories[button_index] == inventory_filter else Color.WHITE
	for child in inventory_grid.get_children():
		child.queue_free()
	for item_index in MineplusInventory.visible_item_indices(inventory_filter):
		var block_type := MineplusInventory.block_type_for_item(item_index)
		var action_text := "TOQUE PARA EQUIPAR" if block_type >= 0 else "MATÉRIA-PRIMA"
		var item_button := Button.new()
		item_button.text = "%s\n%d / %d\n%s" % [INVENTORY_NAMES[item_index].to_upper(), block_counts[item_index], MAX_STACK_SIZE, action_text]
		var item_width := (inventory_panel_width - 55.0) / float(inventory_grid.columns)
		item_button.custom_minimum_size = Vector2(item_width, 66)
		item_button.add_theme_font_size_override("font_size", 11)
		item_button.add_theme_color_override("font_color", INVENTORY_COLORS[item_index])
		item_button.add_theme_color_override("font_hover_color", Color.WHITE)
		item_button.tooltip_text = "Equipar %s na hotbar" % INVENTORY_NAMES[item_index] if block_type >= 0 else "Madeira é usada na fabricação de tábuas."
		item_button.pressed.connect(_equip_inventory_item.bind(item_index))
		inventory_grid.add_child(item_button)

func _load_settings() -> void:
	var config := MineplusSettings.load_config()
	MineplusSettings.apply_runtime(config)
	if config == null:
		return
	look_sensitivity = clampf(float(config.get_value("controls", "sensitivity", 0.0025)), 0.0008, 0.008)
	configured_fov = clampf(float(config.get_value("graphics", "fov", 78.0)), 60.0, 100.0)
	graphics_quality = clampi(int(config.get_value("graphics", "quality", 1)), 0, 2)
	hud_opacity = clampf(float(config.get_value("hud", "opacity", 1.0)), 0.45, 1.0)
	selected_skin = clampi(int(config.get_value("appearance", "skin", 0)), 0, SKIN_COLORS.size() - 1)
	world_slot = clampi(int(config.get_value("world", "slot", 1)), 1, 3)

func _create_pause_menu(hud: Control) -> void:
	var pause_button := Button.new()
	pause_button.text = "PAUSAR"
	pause_button.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	pause_button.position = Vector2(-156, 68)
	pause_button.custom_minimum_size = Vector2(138, 42)
	pause_button.pressed.connect(_toggle_pause)
	hud.add_child(pause_button)
	pause_layer = CanvasLayer.new()
	pause_layer.process_mode = Node.PROCESS_MODE_ALWAYS
	pause_layer.visible = false
	pause_layer.layer = 10
	add_child(pause_layer)
	var shade := ColorRect.new()
	shade.color = Color(0.015, 0.02, 0.03, 0.88)
	shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	pause_layer.add_child(shade)
	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	pause_layer.add_child(center)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(minf(320.0, get_viewport().get_visible_rect().size.x - 32.0), 270)
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = Color("#101720")
	panel_style.border_color = Color("#8ee3b1")
	panel_style.set_border_width_all(1)
	panel_style.set_corner_radius_all(18)
	panel_style.content_margin_left = 24
	panel_style.content_margin_right = 24
	panel_style.content_margin_top = 22
	panel_style.content_margin_bottom = 22
	panel.add_theme_stylebox_override("panel", panel_style)
	center.add_child(panel)
	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 12)
	panel.add_child(stack)
	var title := Label.new()
	title.text = "JOGO PAUSADO"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 22)
	title.add_theme_color_override("font_color", Color("#8ee3b1"))
	stack.add_child(title)
	var resume := Button.new()
	resume.text = "CONTINUAR"
	resume.custom_minimum_size.y = 46
	resume.pressed.connect(_resume_game)
	stack.add_child(resume)
	var save := Button.new()
	save.text = "SALVAR E CONTINUAR"
	save.custom_minimum_size.y = 46
	save.pressed.connect(func():
		_save_world()
		_resume_game()
	)
	stack.add_child(save)
	var lobby := Button.new()
	lobby.text = "SALVAR E VOLTAR AO LOBBY"
	lobby.custom_minimum_size.y = 46
	lobby.pressed.connect(func():
		_save_world()
		get_tree().paused = false
		get_tree().change_scene_to_file("res://scenes/main_menu.tscn")
	)
	stack.add_child(lobby)

func _toggle_pause() -> void:
	touch_look_active = false
	touch_look_index = -1
	if get_tree().paused:
		_resume_game()
	else:
		pause_layer.visible = true
		get_tree().paused = true
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func _resume_game() -> void:
	get_tree().paused = false
	if pause_layer:
		pause_layer.visible = false
	if not OS.has_feature("mobile"):
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _add_save_button(hud: Control) -> void:
	var save_button := Button.new()
	save_button.text = "SALVAR MUNDO"
	save_button.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	save_button.position = Vector2(-156, 18)
	save_button.custom_minimum_size = Vector2(138, 42)
	save_button.pressed.connect(_save_world)
	hud.add_child(save_button)


func _add_craft_button(hud: Control) -> void:
	var craft_button := Button.new()
	craft_button.text = "FABRICAR"
	craft_button.set_anchors_preset(Control.PRESET_TOP_LEFT)
	craft_button.position = Vector2(22, 76)
	craft_button.custom_minimum_size = Vector2(132, 42)
	craft_button.pressed.connect(_show_crafting)
	hud.add_child(craft_button)

func _show_crafting() -> void:
	var dialog := Window.new()
	dialog.title = "FABRICAÇÃO"
	var viewport_size := get_viewport().get_visible_rect().size
	dialog.size = Vector2i(int(minf(520.0, viewport_size.x - 24.0)), int(maxf(240.0, minf(440.0, viewport_size.y - 40.0))))
	dialog.close_requested.connect(dialog.queue_free)
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 18)
	margin.add_theme_constant_override("margin_right", 18)
	margin.add_theme_constant_override("margin_top", 18)
	margin.add_theme_constant_override("margin_bottom", 18)
	dialog.add_child(margin)
	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 10)
	margin.add_child(stack)
	var description := Label.new()
	description.text = "Receitas transacionais: os ingredientes só são consumidos quando a fabricação pode ser concluída."
	description.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	stack.add_child(description)
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	stack.add_child(scroll)
	var recipe_list := VBoxContainer.new()
	recipe_list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	recipe_list.add_theme_constant_override("separation", 10)
	scroll.add_child(recipe_list)
	for recipe_id in MineplusCrafting.recipe_ids():
		var recipe_card := PanelContainer.new()
		var card_stack := VBoxContainer.new()
		card_stack.add_theme_constant_override("separation", 5)
		recipe_card.add_child(card_stack)
		var recipe_title := Label.new()
		recipe_title.text = MineplusCrafting.recipe_title(recipe_id)
		recipe_title.add_theme_font_size_override("font_size", 15)
		recipe_title.add_theme_color_override("font_color", Color("#8ee3b1"))
		card_stack.add_child(recipe_title)
		var requirements := Label.new()
		requirements.text = MineplusCrafting.recipe_description(recipe_id)
		requirements.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		card_stack.add_child(requirements)
		var availability := MineplusCrafting.can_craft(recipe_id, block_counts, pickaxe_durability, axe_durability)
		var craft_button := Button.new()
		craft_button.custom_minimum_size.y = 40
		craft_button.disabled = not bool(availability.get("ok", false))
		craft_button.text = "FABRICAR" if craft_button.disabled == false else str(availability.get("reason", "Indisponível"))
		craft_button.pressed.connect(_craft_recipe.bind(recipe_id, dialog))
		card_stack.add_child(craft_button)
		recipe_list.add_child(recipe_card)
	var close_button := Button.new()
	close_button.text = "FECHAR"
	close_button.pressed.connect(dialog.queue_free)
	stack.add_child(close_button)
	add_child(dialog)
	dialog.popup_centered()

func _craft_recipe(recipe_id: String, dialog: Window) -> void:
	var result := MineplusCrafting.craft(recipe_id, block_counts, pickaxe_durability, axe_durability)
	if not bool(result.get("ok", false)):
		if status_label:
			status_label.text = str(result.get("reason", "Não foi possível fabricar."))
		return
	var crafted_inventory: Array = result["inventory"]
	for index in range(INVENTORY_NAMES.size()):
		block_counts[index] = int(crafted_inventory[index])
	pickaxe_durability = int(result["pickaxe_durability"])
	axe_durability = int(result["axe_durability"])
	if recipe_id == "pickaxe":
		selected_tool = MineplusEquipment.TOOL_PICKAXE
	elif recipe_id == "axe":
		selected_tool = MineplusEquipment.TOOL_AXE
	_update_tool_visuals()
	if dialog:
		dialog.queue_free()
	_update_hotbar()
	_save_world(true)
	if status_label:
		status_label.text = "Fabricação concluída: %s." % MineplusCrafting.recipe_title(recipe_id)

func _process(delta: float) -> void:
	_update_environment(delta)
	autosave_elapsed += delta
	if autosave_elapsed >= AUTOSAVE_INTERVAL:
		autosave_elapsed = 0.0
		_save_world(true)

func _update_environment(delta: float) -> void:
	if world_environment == null or sky_material == null or sun_light == null:
		return
	day_clock = fposmod(day_clock + delta / DAY_LENGTH_SECONDS, 1.0)
	var phase := day_clock * TAU - PI * 0.5
	var sun_height := sin(phase)
	var daylight := clampf((sun_height + 0.12) / 0.82, 0.0, 1.0)
	var twilight := clampf(1.0 - absf(sun_height) * 3.2, 0.0, 1.0) * (1.0 - daylight)
	var night_top := Color("#111a30")
	var day_top := Color("#557fa0")
	var night_horizon := Color("#252c43")
	var day_horizon := Color("#b4c6c4")
	var dusk_horizon := Color("#c17d68")
	sky_material.sky_top_color = night_top.lerp(day_top, daylight)
	sky_material.sky_horizon_color = night_horizon.lerp(day_horizon, daylight).lerp(dusk_horizon, twilight * 0.82)
	sky_material.ground_bottom_color = Color("#10161d").lerp(Color("#46534b"), daylight)
	sky_material.ground_horizon_color = Color("#303844").lerp(Color("#75867b"), daylight).lerp(Color("#795a58"), twilight * 0.55)
	sun_light.rotation_degrees = Vector3(-10.0 - sun_height * 65.0, -28.0, 0.0)
	sun_light.light_energy = lerpf(0.025, 1.25 + float(graphics_quality) * 0.08, pow(daylight, 0.85))
	sun_light.light_color = Color("#e69a78").lerp(Color("#fff0d6"), daylight)
	world_environment.ambient_light_color = Color("#39475f").lerp(Color("#b9cbd0"), daylight)
	world_environment.ambient_light_energy = lerpf(0.22, 0.82 + float(graphics_quality) * 0.06, daylight)
	world_environment.fog_light_color = sky_material.sky_horizon_color.darkened(0.12)

func _world_save_path() -> String:
	if world_slot == 1:
		return "user://mineplus_world.json"
	return "user://mineplus_world_%d.json" % world_slot

func _save_world(silent: bool = false) -> void:
	var saved_blocks: Array = []
	for cell in blocks.keys():
		saved_blocks.append([cell.x, cell.y, cell.z, int(blocks[cell].get_meta("block_type"))])
	var player_position := [0.0, 13.0, 5.0]
	var player_rotation_y := 0.0
	var camera_pitch := 0.0
	if player != null and camera != null:
		player_position = [player.position.x, player.position.y, player.position.z]
		player_rotation_y = player.rotation.y
		camera_pitch = camera.rotation.x
	var payload := {"version": MineplusWorldSave.CURRENT_VERSION, "selected_tool": selected_tool, "world_seed": chunk_streamer.generator.world_seed if chunk_streamer else 4839201, "terrain_edits": chunk_streamer.export_edits() if chunk_streamer else [], "blocks": saved_blocks, "inventory": block_counts, "pickaxe_durability": pickaxe_durability, "axe_durability": axe_durability, "player_position": player_position, "player_rotation_y": player_rotation_y, "camera_pitch": camera_pitch, "day_clock": day_clock, "selected_block": selected_block}
	var saved := MineplusWorldSave.save_world(_world_save_path(), payload)
	if saved:
		if not silent and status_label:
			status_label.text = "Mundo salvo com segurança neste dispositivo."
	elif not silent and status_label:
		status_label.text = "Falha ao salvar. O arquivo anterior foi preservado."

func _load_world() -> void:
	var parsed := MineplusWorldSave.load_world(_world_save_path())
	if parsed.is_empty():
		return
	for body in blocks.values():
		body.queue_free()
	blocks.clear()
	if chunk_streamer:
		chunk_streamer.generator = MineplusWorldGen.new(int(parsed["world_seed"]))
		chunk_streamer.import_edits(parsed["terrain_edits"])
	pickaxe_durability = int(parsed["pickaxe_durability"])
	axe_durability = int(parsed["axe_durability"])
	selected_tool = MineplusEquipment.normalize_selected_tool(parsed.get("selected_tool", MineplusEquipment.TOOL_NONE), pickaxe_durability, axe_durability)
	var saved_position: Array = parsed["player_position"]
	saved_player_position = Vector3(float(saved_position[0]), float(saved_position[1]), float(saved_position[2]))
	saved_player_rotation_y = float(parsed["player_rotation_y"])
	saved_camera_pitch = float(parsed["camera_pitch"])
	day_clock = float(parsed.get("day_clock", 0.5))
	_update_environment(0.0)
	saved_selected_block = int(parsed["selected_block"])
	for i in range(INVENTORY_NAMES.size()):
		block_counts[i] = int(parsed["inventory"][i])
	for entry in parsed["blocks"]:
		_add_block(Vector3i(int(entry[0]), int(entry[1]), int(entry[2])), int(entry[3]))
	if status_label:
		status_label.text = "Mundo salvo carregado."

func _add_hold_button(parent: HBoxContainer, caption: String, action: String, ui_scale: float) -> void:
	var button := Button.new()
	button.text = caption
	button.custom_minimum_size = Vector2(54, 54) * ui_scale
	button.button_down.connect(func(): Input.action_press(action))
	button.button_up.connect(func(): Input.action_release(action))
	parent.add_child(button)

func _update_hotbar() -> void:
	if hotbar_label:
		var inventory_index := MineplusCrafting.inventory_index_for_block(selected_block)
		hotbar_label.text = BLOCK_NAMES[selected_block].to_upper() + "  ×  " + str(block_counts[inventory_index])
	for i in range(hotbar_buttons.size()):
		hotbar_buttons[i].modulate = Color("#8ee3b1") if i == selected_block else Color("#ffffff")
	_refresh_inventory()

func _physics_process(delta: float) -> void:
	if player == null or get_tree().paused:
		return
	var input_dir := Vector2.ZERO
	input_dir.x = Input.get_action_strength("move_right") - Input.get_action_strength("move_left")
	input_dir.y = Input.get_action_strength("move_back") - Input.get_action_strength("move_forward")
	if Input.is_key_pressed(KEY_D): input_dir.x += 1.0
	if Input.is_key_pressed(KEY_A): input_dir.x -= 1.0
	if Input.is_key_pressed(KEY_S): input_dir.y += 1.0
	if Input.is_key_pressed(KEY_W): input_dir.y -= 1.0
	input_dir = input_dir.normalized()
	var direction := (player.transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()
	var speed := 5.2
	player.velocity.x = direction.x * speed
	player.velocity.z = direction.z * speed
	if not player.is_on_floor():
		player.velocity.y -= 22.0 * delta
	elif Input.is_action_just_pressed("jump") or Input.is_key_pressed(KEY_SPACE):
		player.velocity.y = 7.0
	player.move_and_slide()
	if player.global_position.y < -20.0:
		_respawn_player()
		return
	var horizontal_speed := Vector2(player.velocity.x, player.velocity.z).length()
	var moving_on_ground := player.is_on_floor() and horizontal_speed > 0.15
	var movement_amount := clampf(horizontal_speed / 5.2, 0.0, 1.0) if moving_on_ground else 0.0
	walk_animation_time += delta * (10.0 if moving_on_ground else 1.8)
	if moving_on_ground:
		camera.position.y = camera_base_height + sin(walk_animation_time) * 0.035
	else:
		camera.position.y = lerpf(camera.position.y, camera_base_height, minf(delta * 8.0, 1.0))
	_update_first_person_animation(maxf(movement_amount, 0.10))

func _respawn_player() -> void:
	if player == null:
		return
	var respawn_x := 0
	var respawn_z := 5
	var respawn_height := int(chunk_streamer.generator.surface_at(respawn_x, respawn_z)[0]) if chunk_streamer else 11
	player.global_position = Vector3(respawn_x, float(respawn_height) + 2.2, respawn_z)
	player.velocity = Vector3.ZERO
	player.rotation.y = 0.0
	pitch = 0.0
	if camera:
		camera.rotation = Vector3.ZERO
		camera.position.y = camera_base_height
	if status_label:
		status_label.text = "Você caiu do mundo e voltou ao ponto inicial."


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		_rotate_view(event.relative)
	elif event is InputEventScreenDrag:
		if touch_look_active and _touch_drag_matches(touch_look_index, event.index):
			_rotate_view(event.relative)
	elif event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			_interact(false)
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			_interact(true)
	elif event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			_toggle_pause()
		elif event.keycode == KEY_E:
			_toggle_inventory()
		elif event.keycode >= KEY_1 and event.keycode <= KEY_7:
			_select_block(event.keycode - KEY_1)
		elif event.keycode == KEY_8:
			_set_selected_tool(MineplusEquipment.TOOL_PICKAXE)
		elif event.keycode == KEY_9:
			_set_selected_tool(MineplusEquipment.TOOL_AXE)
		elif event.keycode == KEY_0:
			_set_selected_tool(MineplusEquipment.TOOL_NONE)
		elif event.keycode == KEY_SPACE:
			Input.action_press("jump")
	elif event is InputEventScreenTouch:
		if event.pressed:
			if _touch_starts_look(event.position, get_viewport().get_visible_rect().size):
				touch_look_active = true
				touch_look_index = event.index
		elif event.index == touch_look_index:
			touch_look_active = false
			touch_look_index = -1

static func _touch_starts_look(position: Vector2, viewport_size: Vector2) -> bool:
	if viewport_size.x <= 0.0 or viewport_size.y <= 0.0:
		return false
	return position.x > viewport_size.x * 0.48 and position.y >= 0.0 and position.y < viewport_size.y * 0.78

static func _touch_drag_matches(active_index: int, event_index: int) -> bool:
	return active_index >= 0 and event_index == active_index

func _rotate_view(relative: Vector2) -> void:
	player.rotate_y(-relative.x * look_sensitivity)
	pitch = clampf(pitch - relative.y * look_sensitivity, -1.45, 1.45)
	camera.rotation.x = pitch

func _terrain_drop_index(terrain_type: int) -> int:
	match terrain_type:
		MineplusWorldGen.Block.GRASS: return 0
		MineplusWorldGen.Block.WOOD: return 6
		MineplusWorldGen.Block.DIRT, MineplusWorldGen.Block.SNOW, MineplusWorldGen.Block.ICE: return 1
		MineplusWorldGen.Block.STONE, MineplusWorldGen.Block.BASALT: return 2
		MineplusWorldGen.Block.SAND: return 3
		MineplusWorldGen.Block.MOSS: return 4
		MineplusWorldGen.Block.BRICK: return 5
	return -1

static func _required_tool_for_inventory_item(item_index: int) -> String:
	match item_index:
		2, 5: return "pickaxe"
		6, 7: return "axe"
	return ""

static func _required_tool_for_block(block_type: int) -> String:
	return _required_tool_for_inventory_item(MineplusCrafting.inventory_index_for_block(block_type))

func _interact(place: bool) -> void:
	var origin := camera.global_position
	var end := origin + (-camera.global_transform.basis.z * 6.0)
	var query := PhysicsRayQueryParameters3D.create(origin, end)
	query.exclude = [player]
	var hit := get_world_3d().direct_space_state.intersect_ray(query)
	if hit.is_empty():
		return
	var body = hit.collider
	var terrain_hit: bool = body.has_meta("chunk") and chunk_streamer != null
	if not body.has_meta("cell") and not terrain_hit:
		return
	var cell: Vector3i
	if body.has_meta("cell"):
		cell = body.get_meta("cell")
	elif terrain_hit:
		cell = chunk_streamer.cell_from_hit(hit)
	else:
		return
	if place:
		var normal: Vector3 = hit.normal
		var target := cell + Vector3i(roundi(normal.x), roundi(normal.y), roundi(normal.z))
		var place_inventory_index := MineplusCrafting.inventory_index_for_block(selected_block)
		if place_inventory_index < 0 or target.y < 1 or blocks.has(target) or block_counts[place_inventory_index] <= 0:
			return
		var target_center := Vector3(target) * BLOCK_SIZE
		var player_center := player.global_position + Vector3.UP * 0.9
		if absf(target_center.x - player_center.x) < 0.68 and absf(target_center.z - player_center.z) < 0.68 and absf(target_center.y - player_center.y) < 1.35:
			if status_label:
				status_label.text = "Não é possível colocar um bloco dentro do jogador."
			return
		_add_block(target, selected_block)
		block_counts[place_inventory_index] -= 1
		_update_hotbar()
	else:
		if terrain_hit:
			var terrain_type := chunk_streamer._block_type(cell.x, cell.z)
			var drop_index := _terrain_drop_index(terrain_type)
			if drop_index < 0:
				return
			if block_counts[drop_index] >= MAX_STACK_SIZE:
				if status_label: status_label.text = "Pilha cheia: máximo de %d por material." % MAX_STACK_SIZE
				return
			var terrain_required_tool := _required_tool_for_inventory_item(drop_index)
			var tool_check := MineplusEquipment.can_use(terrain_required_tool, selected_tool, pickaxe_durability, axe_durability)
			if not bool(tool_check.get("ok", false)):
				if status_label:
					status_label.text = str(tool_check.get("reason", "Ferramenta indisponível."))
				return
			var needs_pickaxe := terrain_required_tool == MineplusEquipment.TOOL_PICKAXE
			var needs_axe := terrain_required_tool == MineplusEquipment.TOOL_AXE
			chunk_streamer.mine_surface(cell)
			if needs_pickaxe:
				pickaxe_durability -= 1
				_animate_pickaxe_swing()
				if pickaxe_durability == 0:
					selected_tool = MineplusEquipment.TOOL_NONE
			elif needs_axe:
				axe_durability -= 1
				_animate_axe_swing()
				if axe_durability == 0:
					selected_tool = MineplusEquipment.TOOL_NONE
			_update_tool_visuals()
			block_counts[drop_index] += 1
			_update_hotbar()
			if needs_axe and axe_durability == 0 and status_label:
				status_label.text = "Seu machado quebrou. Fabrique outro na bancada."
			elif needs_pickaxe and pickaxe_durability == 0 and status_label:
				status_label.text = "Sua picareta quebrou. Fabrique outra na bancada."
			_save_world(true)
			return
		if cell.y >= 0:
			var block_type := int(body.get_meta("block_type"))
			var drop_index := MineplusCrafting.inventory_index_for_block(block_type)
			if drop_index < 0:
				return
			if block_counts[drop_index] >= MAX_STACK_SIZE:
				if status_label:
					status_label.text = "Pilha cheia: máximo de %d por material." % MAX_STACK_SIZE
				return
			var placed_required_tool := _required_tool_for_block(block_type)
			var placed_tool_check := MineplusEquipment.can_use(placed_required_tool, selected_tool, pickaxe_durability, axe_durability)
			if not bool(placed_tool_check.get("ok", false)):
				if status_label:
					status_label.text = str(placed_tool_check.get("reason", "Ferramenta indisponível."))
				return
			var used_pickaxe := placed_required_tool == MineplusEquipment.TOOL_PICKAXE
			var used_axe := placed_required_tool == MineplusEquipment.TOOL_AXE
			if used_pickaxe:
				pickaxe_durability -= 1
				_animate_pickaxe_swing()
				if pickaxe_durability == 0:
					selected_tool = MineplusEquipment.TOOL_NONE
			elif used_axe:
				axe_durability -= 1
				_animate_axe_swing()
				if axe_durability == 0:
					selected_tool = MineplusEquipment.TOOL_NONE
			_update_tool_visuals()
			block_counts[drop_index] += 1
			_remove_block(cell)
			_update_hotbar()
			_refresh_inventory()
			_save_world(true)
			if used_pickaxe and pickaxe_durability == 0 and status_label:
				status_label.text = "Sua picareta quebrou. Fabrique outra na bancada."
			elif used_axe and axe_durability == 0 and status_label:
				status_label.text = "Seu machado quebrou. Fabrique outro na bancada."

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_APPLICATION_PAUSED:
		_save_world()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

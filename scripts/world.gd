extends Node3D

const WORLD_RADIUS := 9
const BLOCK_SIZE := 1.0
const MAX_STACK_SIZE := 999
const BLOCK_COLORS := [
	Color("#79b85a"), Color("#8b6548"), Color("#858b96"), Color("#c6ad72"), Color("#4e9d83"), Color("#b86f62")
]
const BLOCK_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo"]
const INVENTORY_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo", "Madeira"]
const INVENTORY_COLORS := [Color("#79b85a"), Color("#8b6548"), Color("#858b96"), Color("#c6ad72"), Color("#4e9d83"), Color("#b86f62"), Color("#b88754")]

var blocks: Dictionary = {}
var chunk_streamer: MineplusChunkStreamer
var selected_block := 0
var block_counts: Array[int] = [64, 64, 32, 32, 16, 0, 0]
var player: CharacterBody3D
var camera: Camera3D
var pickaxe_visual: Node3D
var hotbar_label: Label
var hotbar_buttons: Array[Button] = []
var status_label: Label
var pitch := 0.0
var touch_look_active := false
var inventory_panel: PanelContainer
var inventory_grid: GridContainer
var inventory_panel_width := 460.0
var pause_layer: CanvasLayer
var inventory_tool_label: Label
var look_sensitivity := 0.0025
var configured_fov := 78.0
var hud_opacity := 1.0
var graphics_quality := 1
var pickaxe_durability := 0
var axe_durability := 0
var axe_visual: Node3D
var block_textures: Array[Texture2D] = []
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
	var arm_material := StandardMaterial3D.new()
	arm_material.albedo_color = SKIN_COLORS[selected_skin]
	arm_material.roughness = 0.85
	for side in [-1.0, 1.0]:
		var arm := MeshInstance3D.new()
		var arm_mesh := BoxMesh.new()
		arm_mesh.size = Vector3(0.16, 0.42, 0.16)
		arm.mesh = arm_mesh
		arm.material_override = arm_material
		arm.position = Vector3(side * 0.34, -0.43, -0.52)
		arm.rotation_degrees.z = side * -12.0
		camera.add_child(arm)

func _create_pickaxe_visual() -> void:
	pickaxe_visual = Node3D.new()
	pickaxe_visual.position = Vector3(0.38, -0.36, -0.62)
	pickaxe_visual.rotation_degrees = Vector3(-18, -24, -18)
	pickaxe_visual.visible = pickaxe_durability > 0
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
	axe_visual.visible = axe_durability > 0
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
	hud.modulate.a = hud_opacity
	add_child(hud)
	var top := Label.new()
	top.text = "MINEPLUS  /  SURVIVAL PROTOTYPE"
	top.position = Vector2(22, 18)
	top.add_theme_color_override("font_color", Color("#e9f2f2"))
	top.add_theme_font_size_override("font_size", 15)
	hud.add_child(top)
	status_label = Label.new()
	status_label.text = "WASD mover  •  Mouse olhar  •  Clique quebrar/colocar"
	status_label.position = Vector2(22, 44)
	status_label.add_theme_color_override("font_color", Color("#c2d0d0"))
	status_label.add_theme_font_size_override("font_size", 12)
	hud.add_child(status_label)
	var crosshair := Label.new()
	crosshair.text = "+"
	crosshair.set_anchors_preset(Control.PRESET_CENTER)
	crosshair.position = Vector2(-7, -15)
	crosshair.add_theme_font_size_override("font_size", 26)
	crosshair.add_theme_color_override("font_color", Color("#ffffff"))
	hud.add_child(crosshair)
	hotbar_label = Label.new()
	hotbar_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	var hud_width := minf(500.0, get_viewport().get_visible_rect().size.x - 20.0)
	hotbar_label.position = Vector2(-hud_width * 0.5, -74)
	hotbar_label.size = Vector2(hud_width, 44)
	hotbar_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hotbar_label.add_theme_font_size_override("font_size", 17)
	hotbar_label.add_theme_color_override("font_color", Color("#f1f5f2"))
	hud.add_child(hotbar_label)
	_update_hotbar()
	if OS.has_feature("mobile"):
		_add_touch_controls(hud)
	_add_hotbar_buttons(hud)
	_add_save_button(hud)
	_add_craft_button(hud)
	_add_inventory_button(hud)
	_create_inventory_panel(hud)
	_create_pause_menu(hud)

func _add_touch_controls(hud: CanvasLayer) -> void:
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

func _add_hotbar_buttons(hud: CanvasLayer) -> void:
	var bar := HBoxContainer.new()
	bar.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	var viewport_width := get_viewport().get_visible_rect().size.x
	var bar_width := minf(440.0, viewport_width - 16.0)
	bar.position = Vector2(-bar_width * 0.5, -126)
	bar.size = Vector2(bar_width, 42)
	bar.alignment = BoxContainer.ALIGNMENT_CENTER
	bar.add_theme_constant_override("separation", 5)
	hud.add_child(bar)
	for i in range(BLOCK_NAMES.size()):
		var slot := Button.new()
		slot.text = str(i + 1) + " " + BLOCK_NAMES[i].substr(0, 3).to_upper()
		slot.custom_minimum_size = Vector2((bar_width - 25.0) / 6.0, 38)
		slot.add_theme_font_size_override("font_size", 9)
		slot.pressed.connect(_select_block.bind(i))
		bar.add_child(slot)
		hotbar_buttons.append(slot)
	_update_hotbar()

func _select_block(index: int) -> void:
	selected_block = clampi(index, 0, BLOCK_NAMES.size() - 1)
	_update_hotbar()

func _add_inventory_button(hud: CanvasLayer) -> void:
	var button := Button.new()
	button.text = "INVENTÁRIO"
	button.set_anchors_preset(Control.PRESET_TOP_LEFT)
	button.position = Vector2(166, 76)
	button.custom_minimum_size = Vector2(132, 42)
	button.pressed.connect(_toggle_inventory)
	hud.add_child(button)

func _create_inventory_panel(hud: CanvasLayer) -> void:
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
	stack.add_child(inventory_tool_label)
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
	inventory_panel.visible = not inventory_panel.visible
	if inventory_panel.visible:
		_refresh_inventory()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	elif not OS.has_feature("mobile"):
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _refresh_inventory() -> void:
	if inventory_grid == null:
		return
	if inventory_tool_label:
		inventory_tool_label.text = "PICARETA %d/40  •  MACHADO %d/40" % [pickaxe_durability, axe_durability]
	for child in inventory_grid.get_children():
		child.queue_free()
	for i in range(INVENTORY_NAMES.size()):
		var item := PanelContainer.new()
		var item_style := StyleBoxFlat.new()
		item_style.bg_color = Color(1, 1, 1, 0.055)
		item_style.set_corner_radius_all(9)
		item.add_theme_stylebox_override("panel", item_style)
		var label := Label.new()
		label.text = "  %s\n  Quantidade: %d" % [INVENTORY_NAMES[i], block_counts[i]]
		var item_width := (inventory_panel_width - 55.0) / float(inventory_grid.columns)
		label.custom_minimum_size = Vector2(item_width, 48)
		label.add_theme_color_override("font_color", INVENTORY_COLORS[i])
		item.add_child(label)
		inventory_grid.add_child(item)

func _load_settings() -> void:
	var config := ConfigFile.new()
	if config.load("user://mineplus_settings.cfg") == OK:
		look_sensitivity = clampf(float(config.get_value("controls", "sensitivity", 0.0025)), 0.0008, 0.008)
		configured_fov = clampf(float(config.get_value("graphics", "fov", 78.0)), 60.0, 100.0)
		graphics_quality = clampi(int(config.get_value("graphics", "quality", 1)), 0, 2)
		hud_opacity = clampf(float(config.get_value("hud", "opacity", 1.0)), 0.45, 1.0)
		selected_skin = clampi(int(config.get_value("appearance", "skin", 0)), 0, SKIN_COLORS.size() - 1)
		world_slot = clampi(int(config.get_value("world", "slot", 1)), 1, 3)

func _create_pause_menu(hud: CanvasLayer) -> void:
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

func _add_save_button(hud: CanvasLayer) -> void:
	var save_button := Button.new()
	save_button.text = "SALVAR MUNDO"
	save_button.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	save_button.position = Vector2(-156, 18)
	save_button.custom_minimum_size = Vector2(138, 42)
	save_button.pressed.connect(_save_world)
	hud.add_child(save_button)


func _add_craft_button(hud: CanvasLayer) -> void:
	var craft_button := Button.new()
	craft_button.text = "FABRICAR"
	craft_button.set_anchors_preset(Control.PRESET_TOP_LEFT)
	craft_button.position = Vector2(22, 76)
	craft_button.custom_minimum_size = Vector2(132, 42)
	craft_button.pressed.connect(_show_crafting)
	hud.add_child(craft_button)

func _show_crafting() -> void:
	var dialog := Window.new()
	dialog.title = "BANCADA DE FABRICAÇÃO"
	var viewport_size := get_viewport().get_visible_rect().size
	dialog.size = Vector2i(int(minf(420.0, viewport_size.x - 24.0)), int(minf(330.0, viewport_size.y - 40.0)))
	dialog.close_requested.connect(dialog.hide)
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	margin.add_theme_constant_override("margin_left", 18)
	margin.add_theme_constant_override("margin_right", 18)
	margin.add_theme_constant_override("margin_top", 18)
	margin.add_theme_constant_override("margin_bottom", 18)
	dialog.add_child(margin)
	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 12)
	margin.add_child(stack)
	var description := Label.new()
	description.text = "Escolha uma receita. Os materiais são descontados do inventário."
	description.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	stack.add_child(description)
	var bricks_button := Button.new()
	bricks_button.text = "2 Terra + 1 Pedra  →  2 Tijolos"
	bricks_button.pressed.connect(func():
		dialog.hide()
		_craft_bricks()
	)
	stack.add_child(bricks_button)
	var pickaxe_button := Button.new()
	pickaxe_button.text = "3 Pedras + 2 Tijolos  →  Picareta (40 usos)"
	pickaxe_button.pressed.connect(func():
		dialog.hide()
		_craft_pickaxe()
	)
	stack.add_child(pickaxe_button)
	var axe_button := Button.new()
	axe_button.text = "2 Pedras + 2 Terras  →  Machado de pedra (40 usos)"
	axe_button.pressed.connect(func():
		dialog.hide()
		_craft_axe()
	)
	stack.add_child(axe_button)
	var close_button := Button.new()
	close_button.text = "FECHAR"
	close_button.pressed.connect(dialog.hide)
	stack.add_child(close_button)
	add_child(dialog)
	dialog.popup_centered()

func _craft_pickaxe() -> void:
	if pickaxe_durability > 0:
		status_label.text = "Você já possui uma picareta com %d de durabilidade." % pickaxe_durability
		return
	if block_counts[2] < 3 or block_counts[5] < 2:
		status_label.text = "Picareta: faltam 3 Pedras e 2 Tijolos."
		return
	block_counts[2] -= 3
	block_counts[5] -= 2
	pickaxe_durability = 40
	if pickaxe_visual:
		pickaxe_visual.visible = true
	_update_hotbar()
	_refresh_inventory()
	_save_world(true)
	status_label.text = "Picareta fabricada: 40 usos."

func _craft_axe() -> void:
	if axe_durability > 0:
		status_label.text = "Você já possui um machado com %d de durabilidade." % axe_durability
		return
	if block_counts[2] < 2 or block_counts[1] < 2:
		status_label.text = "Machado: faltam 2 Pedras e 2 Terras."
		return
	block_counts[2] -= 2
	block_counts[1] -= 2
	axe_durability = 40
	if axe_visual:
		axe_visual.visible = true
	_update_hotbar()
	_refresh_inventory()
	_save_world(true)
	status_label.text = "Machado de pedra fabricado: 40 usos."

func _craft_bricks() -> void:
	if block_counts[1] < 2 or block_counts[2] < 1 or block_counts[5] > MAX_STACK_SIZE - 2:
		status_label.text = "Faltam materiais: 2 Terra e 1 Pedra."
		return
	block_counts[1] -= 2
	block_counts[2] -= 1
	block_counts[5] += 2
	_update_hotbar()
	_refresh_inventory()
	_save_world(true)
	status_label.text = "Fabricação concluída: +2 Tijolos."

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
	var payload := {"version": MineplusWorldSave.CURRENT_VERSION, "world_seed": chunk_streamer.generator.world_seed if chunk_streamer else 4839201, "terrain_edits": chunk_streamer.export_edits() if chunk_streamer else [], "blocks": saved_blocks, "inventory": block_counts, "pickaxe_durability": pickaxe_durability, "axe_durability": axe_durability, "player_position": player_position, "player_rotation_y": player_rotation_y, "camera_pitch": camera_pitch, "selected_block": selected_block}
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
	var saved_position: Array = parsed["player_position"]
	saved_player_position = Vector3(float(saved_position[0]), float(saved_position[1]), float(saved_position[2]))
	saved_player_rotation_y = float(parsed["player_rotation_y"])
	saved_camera_pitch = float(parsed["camera_pitch"])
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
		hotbar_label.text = BLOCK_NAMES[selected_block].to_upper() + "  ×  " + str(block_counts[selected_block])
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
	if player.is_on_floor() and Vector2(player.velocity.x, player.velocity.z).length() > 0.15:
		walk_animation_time += delta * 10.0
		camera.position.y = camera_base_height + sin(walk_animation_time) * 0.035
	else:
		camera.position.y = lerpf(camera.position.y, camera_base_height, minf(delta * 8.0, 1.0))

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
		if event.position.x > get_viewport().get_visible_rect().size.x * 0.32:
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
		elif event.keycode >= KEY_1 and event.keycode <= KEY_6:
			_select_block(event.keycode - KEY_1)
		elif event.keycode == KEY_SPACE:
			Input.action_press("jump")
	elif event is InputEventScreenTouch and event.pressed:
		if event.position.x > get_viewport().get_visible_rect().size.x * 0.48 and event.position.y < get_viewport().get_visible_rect().size.y * 0.78:
			_interact(false)

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
		if target.y < 1 or blocks.has(target) or block_counts[selected_block] <= 0:
			return
		var target_center := Vector3(target) * BLOCK_SIZE
		var player_center := player.global_position + Vector3.UP * 0.9
		if absf(target_center.x - player_center.x) < 0.68 and absf(target_center.z - player_center.z) < 0.68 and absf(target_center.y - player_center.y) < 1.35:
			if status_label:
				status_label.text = "Não é possível colocar um bloco dentro do jogador."
			return
		_add_block(target, selected_block)
		block_counts[selected_block] -= 1
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
			var needs_pickaxe := drop_index == 2 or drop_index == 5
			var needs_axe := drop_index == 6
			if needs_pickaxe and pickaxe_durability <= 0:
				if status_label: status_label.text = "Precisa de uma picareta para minerar este bloco."
				return
			if needs_axe and axe_durability <= 0:
				if status_label: status_label.text = "Precisa de um machado para cortar madeira. Fabrique um na bancada."
				return
			chunk_streamer.mine_surface(cell)
			if needs_pickaxe:
				pickaxe_durability -= 1
				_animate_pickaxe_swing()
				if pickaxe_durability == 0 and pickaxe_visual:
					pickaxe_visual.visible = false
			elif needs_axe:
				axe_durability -= 1
				_animate_axe_swing()
				if axe_durability == 0 and axe_visual:
					axe_visual.visible = false
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
			if block_counts[block_type] >= MAX_STACK_SIZE:
				if status_label:
					status_label.text = "Pilha cheia: máximo de %d por material." % MAX_STACK_SIZE
				return
			var used_pickaxe := block_type == 2 or block_type == 5
			if used_pickaxe:
				if pickaxe_durability <= 0:
					if status_label:
						status_label.text = "Precisa de uma picareta para minerar este bloco."
					return
				pickaxe_durability -= 1
				_animate_pickaxe_swing()
				if pickaxe_durability == 0 and pickaxe_visual:
					pickaxe_visual.visible = false
			block_counts[block_type] += 1
			_remove_block(cell)
			_update_hotbar()
			_refresh_inventory()
			if used_pickaxe and pickaxe_durability == 0 and status_label:
				status_label.text = "Sua picareta quebrou. Fabrique outra na bancada."

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_APPLICATION_PAUSED:
		_save_world()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

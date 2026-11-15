extends Node3D

const WORLD_RADIUS := 9
const BLOCK_SIZE := 1.0
const BLOCK_COLORS := [
	Color("#79b85a"), Color("#8b6548"), Color("#858b96"), Color("#c6ad72"), Color("#4e9d83"), Color("#b86f62")
]
const BLOCK_NAMES := ["Grama", "Terra", "Pedra", "Areia", "Musgo", "Tijolo"]

var blocks: Dictionary = {}
var selected_block := 0
var block_counts: Array[int] = [64, 64, 32, 32, 16, 0]
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
var graphics_quality := 1
var pickaxe_durability := 0
var block_textures: Array[Texture2D] = []
var walk_animation_time := 0.0
var camera_base_height := 1.55
var world_slot := 1
var autosave_elapsed := 0.0
const AUTOSAVE_INTERVAL := 45.0
var selected_skin := 0
const SKIN_COLORS := [Color("#d9a47c"), Color("#8b5a3c"), Color("#e7c7a5"), Color("#c8b0e8")]

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_load_settings()
	_setup_environment()
	_setup_block_textures()
	_generate_world()
	_load_world()
	_create_player()
	_create_hud()
	if not OS.has_feature("mobile"):
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED

func _setup_environment() -> void:
	var world_env := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = Sky.new()
	var sky_mat := ProceduralSkyMaterial.new()
	sky_mat.sky_top_color = Color("#263a55")
	sky_mat.sky_horizon_color = Color("#9ab6bd")
	sky_mat.ground_bottom_color = Color("#202b2b")
	sky_mat.ground_horizon_color = Color("#647b76")
	env.sky.sky_material = sky_mat
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("#b9cbd0")
	env.ambient_light_energy = 0.7 + float(graphics_quality) * 0.1
	world_env.environment = env
	add_child(world_env)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52, -28, 0)
	sun.light_energy = 1.0 + float(graphics_quality) * 0.1
	sun.shadow_enabled = graphics_quality >= 2
	add_child(sun)

func _generate_world() -> void:
	for x in range(-WORLD_RADIUS, WORLD_RADIUS + 1):
		for z in range(-WORLD_RADIUS, WORLD_RADIUS + 1):
			_add_block(Vector3i(x, 0, z), 0)
			if (x * 17 + z * 31) % 23 == 0:
				_add_block(Vector3i(x, -1, z), 1)
	for x in range(-2, 3):
		for z in range(-2, 3):
			if abs(x) == 2 or abs(z) == 2:
				_add_block(Vector3i(x, 1, z), 1)
	_add_tree(Vector3i(-5, 1, -4))
	_add_tree(Vector3i(5, 1, -5))
	_add_tree(Vector3i(-6, 1, 5))
	_add_tree(Vector3i(6, 1, 4))

func _add_tree(base: Vector3i) -> void:
	for y in range(3):
		_add_block(base + Vector3i(0, y, 0), 1)
	for x in range(-2, 3):
		for z in range(-2, 3):
			for y in range(2, 4):
				if abs(x) + abs(z) < 4:
					_add_block(base + Vector3i(x, y, z), 4)

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
	player = CharacterBody3D.new()
	player.position = Vector3(0, 2.4, 5)
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
	player.add_child(camera)
	_create_first_person_arms()
	_create_pickaxe_visual()
	add_child(player)

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

func _create_hud() -> void:
	var hud := CanvasLayer.new()
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
	_add_touch_controls(hud)
	_add_hotbar_buttons(hud)
	_add_save_button(hud)
	_add_craft_button(hud)
	_add_inventory_button(hud)
	_create_inventory_panel(hud)
	_create_pause_menu(hud)

func _add_touch_controls(hud: CanvasLayer) -> void:
	var ui_scale := clampf(get_viewport().get_visible_rect().size.x / 720.0, 0.65, 1.0)
	var move_box := HBoxContainer.new()
	move_box.position = Vector2(14, -112 * ui_scale)
	move_box.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	move_box.add_theme_constant_override("separation", 7)
	hud.add_child(move_box)
	_add_hold_button(move_box, "◀", "move_left")
	_add_hold_button(move_box, "▲", "move_forward")
	_add_hold_button(move_box, "▼", "move_back")
	_add_hold_button(move_box, "▶", "move_right")
	var break_button := Button.new()
	break_button.text = "QUEBRAR"
	break_button.position = Vector2(-218 * ui_scale, -112 * ui_scale)
	break_button.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	break_button.custom_minimum_size = Vector2(96, 54) * ui_scale
	break_button.pressed.connect(func(): _interact(false))
	hud.add_child(break_button)
	var place_button := Button.new()
	place_button.text = "COLOCAR"
	place_button.position = Vector2(-322 * ui_scale, -112 * ui_scale)
	place_button.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	place_button.custom_minimum_size = Vector2(96, 54) * ui_scale
	place_button.pressed.connect(func(): _interact(true))
	hud.add_child(place_button)
	var jump := Button.new()
	jump.text = "PULAR"
	jump.position = Vector2(-112 * ui_scale, -112 * ui_scale)
	jump.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	jump.custom_minimum_size = Vector2(88, 54) * ui_scale
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
	inventory_tool_label.text = "PICARETA  •  Durabilidade: %d / 40" % pickaxe_durability
	inventory_tool_label.add_theme_color_override("font_color", Color("#e9d28b"))
	stack.add_child(inventory_tool_label)
	inventory_grid = GridContainer.new()
	inventory_grid.columns = 2 if panel_width >= 420.0 else 1
	inventory_grid.add_theme_constant_override("h_separation", 10)
	inventory_grid.add_theme_constant_override("v_separation", 8)
	stack.add_child(inventory_grid)
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
		inventory_tool_label.text = "PICARETA  •  Durabilidade: %d / 40" % pickaxe_durability
	for child in inventory_grid.get_children():
		child.queue_free()
	for i in range(BLOCK_NAMES.size()):
		var item := PanelContainer.new()
		var item_style := StyleBoxFlat.new()
		item_style.bg_color = Color(1, 1, 1, 0.055)
		item_style.set_corner_radius_all(9)
		item.add_theme_stylebox_override("panel", item_style)
		var label := Label.new()
		label.text = "  %s\n  Quantidade: %d" % [BLOCK_NAMES[i], block_counts[i]]
		var item_width := (inventory_panel_width - 55.0) / float(inventory_grid.columns)
		label.custom_minimum_size = Vector2(item_width, 48)
		label.add_theme_color_override("font_color", BLOCK_COLORS[i])
		item.add_child(label)
		inventory_grid.add_child(item)

func _load_settings() -> void:
	var config := ConfigFile.new()
	if config.load("user://mineplus_settings.cfg") == OK:
		look_sensitivity = clampf(float(config.get_value("controls", "sensitivity", 0.0025)), 0.0008, 0.008)
		configured_fov = clampf(float(config.get_value("graphics", "fov", 78.0)), 60.0, 100.0)
		graphics_quality = clampi(int(config.get_value("graphics", "quality", 1)), 0, 2)
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
	dialog.size = Vector2i(int(minf(420.0, viewport_size.x - 24.0)), int(minf(270.0, viewport_size.y - 40.0)))
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
	status_label.text = "Picareta fabricada: 40 usos."

func _craft_bricks() -> void:
	if block_counts[1] < 2 or block_counts[2] < 1:
		status_label.text = "Faltam materiais: 2 Terra e 1 Pedra."
		return
	block_counts[1] -= 2
	block_counts[2] -= 1
	block_counts[5] += 2
	_update_hotbar()
	_refresh_inventory()
	status_label.text = "Fabricação concluída: +2 Tijolos."

func _process(delta: float) -> void:
	autosave_elapsed += delta
	if autosave_elapsed >= AUTOSAVE_INTERVAL:
		autosave_elapsed = 0.0
		_save_world(true)

func _world_save_path() -> String:
	if world_slot == 1:
		return "user://mineplus_world.json"
	return "user://mineplus_world_%d.json" % world_slot

func _save_world(silent: bool = false) -> void:
	var saved_blocks: Array = []
	for cell in blocks.keys():
		saved_blocks.append([cell.x, cell.y, cell.z, int(blocks[cell].get_meta("block_type"))])
	var payload := {"version": 2, "blocks": saved_blocks, "inventory": block_counts, "pickaxe_durability": pickaxe_durability}
	var file := FileAccess.open(_world_save_path(), FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(payload))
		if not silent and status_label:
			status_label.text = "Mundo salvo neste dispositivo."
	elif not silent and status_label:
		status_label.text = "Não foi possível salvar o mundo."

func _load_world() -> void:
	if not FileAccess.file_exists(_world_save_path()):
		return
	var file := FileAccess.open(_world_save_path(), FileAccess.READ)
	if file == null:
		return
	var parsed = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY or not parsed.has("blocks"):
		return
	for body in blocks.values():
		body.queue_free()
	blocks.clear()
	if parsed.has("pickaxe_durability"):
		pickaxe_durability = maxi(0, int(parsed["pickaxe_durability"]))
	if parsed.has("inventory") and parsed["inventory"] is Array and parsed["inventory"].size() > 0:
		for i in range(mini(BLOCK_NAMES.size(), parsed["inventory"].size())):
			block_counts[i] = maxi(0, int(parsed["inventory"][i]))
	for entry in parsed["blocks"]:
		if entry is Array and entry.size() == 4:
			_add_block(Vector3i(int(entry[0]), int(entry[1]), int(entry[2])), clampi(int(entry[3]), 0, BLOCK_COLORS.size() - 1))
	if status_label:
		status_label.text = "Mundo salvo carregado."

func _add_hold_button(parent: HBoxContainer, caption: String, action: String) -> void:
	var button := Button.new()
	button.text = caption
	button.custom_minimum_size = Vector2(54, 54) * clampf(get_viewport().get_visible_rect().size.x / 720.0, 0.65, 1.0)
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
	if player.is_on_floor() and Vector2(player.velocity.x, player.velocity.z).length() > 0.15:
		walk_animation_time += delta * 10.0
		camera.position.y = camera_base_height + sin(walk_animation_time) * 0.035
	else:
		camera.position.y = lerpf(camera.position.y, camera_base_height, minf(delta * 8.0, 1.0))

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

func _interact(place: bool) -> void:
	var origin := camera.global_position
	var end := origin + (-camera.global_transform.basis.z * 6.0)
	var query := PhysicsRayQueryParameters3D.create(origin, end)
	query.exclude = [player]
	var hit := get_world_3d().direct_space_state.intersect_ray(query)
	if hit.is_empty():
		return
	var body = hit.collider
	if not body.has_meta("cell"):
		return
	var cell: Vector3i = body.get_meta("cell")
	if place:
		var normal: Vector3 = hit.normal
		var target := cell + Vector3i(roundi(normal.x), roundi(normal.y), roundi(normal.z))
		if target.y >= 1 and not blocks.has(target) and block_counts[selected_block] > 0:
			_add_block(target, selected_block)
			block_counts[selected_block] -= 1
			_update_hotbar()
	else:
		if cell.y >= 0:
			var block_type := int(body.get_meta("block_type"))
			if block_type == 2 or block_type == 5:
				if pickaxe_durability <= 0:
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
			if pickaxe_durability == 0:
				status_label.text = "Sua picareta quebrou. Fabrique outra na bancada."

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_APPLICATION_PAUSED:
		_save_world()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

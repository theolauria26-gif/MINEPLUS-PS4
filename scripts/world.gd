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
var hotbar_label: Label
var status_label: Label
var pitch := 0.0
var touch_look_active := false
var inventory_panel: PanelContainer
var inventory_grid: GridContainer
var look_sensitivity := 0.0025
var configured_fov := 78.0

func _ready() -> void:
	_load_settings()
	_setup_environment()
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
	env.ambient_light_energy = 0.75
	world_env.environment = env
	add_child(world_env)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52, -28, 0)
	sun.light_energy = 1.15
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
	material.albedo_color = BLOCK_COLORS[block_type]
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
	camera.position.y = 1.55
	camera.current = true
	camera.fov = configured_fov
	player.add_child(camera)
	add_child(player)

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
	hotbar_label.position = Vector2(-250, -74)
	hotbar_label.size = Vector2(500, 44)
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

func _add_touch_controls(hud: CanvasLayer) -> void:
	var move_box := HBoxContainer.new()
	move_box.position = Vector2(22, -112)
	move_box.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	move_box.add_theme_constant_override("separation", 7)
	hud.add_child(move_box)
	_add_hold_button(move_box, "◀", "move_left")
	_add_hold_button(move_box, "▲", "move_forward")
	_add_hold_button(move_box, "▼", "move_back")
	_add_hold_button(move_box, "▶", "move_right")
	var break_button := Button.new()
	break_button.text = "QUEBRAR"
	break_button.position = Vector2(-218, -112)
	break_button.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	break_button.custom_minimum_size = Vector2(96, 54)
	break_button.pressed.connect(func(): _interact(false))
	hud.add_child(break_button)
	var place_button := Button.new()
	place_button.text = "COLOCAR"
	place_button.position = Vector2(-322, -112)
	place_button.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	place_button.custom_minimum_size = Vector2(96, 54)
	place_button.pressed.connect(func(): _interact(true))
	hud.add_child(place_button)
	var jump := Button.new()
	jump.text = "PULAR"
	jump.position = Vector2(-112, -112)
	jump.set_anchors_preset(Control.PRESET_BOTTOM_RIGHT)
	jump.custom_minimum_size = Vector2(88, 54)
	jump.button_down.connect(func(): Input.action_press("jump"))
	jump.button_up.connect(func(): Input.action_release("jump"))
	hud.add_child(jump)

func _add_hotbar_buttons(hud: CanvasLayer) -> void:
	var bar := HBoxContainer.new()
	bar.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	bar.position = Vector2(-220, -126)
	bar.size = Vector2(440, 42)
	bar.alignment = BoxContainer.ALIGNMENT_CENTER
	bar.add_theme_constant_override("separation", 5)
	hud.add_child(bar)
	for i in range(BLOCK_NAMES.size()):
		var slot := Button.new()
		slot.text = str(i + 1) + " " + BLOCK_NAMES[i]
		slot.custom_minimum_size = Vector2(68, 38)
		slot.add_theme_font_size_override("font_size", 10)
		slot.pressed.connect(_select_block.bind(i))
		bar.add_child(slot)

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
	inventory_panel.position = Vector2(-230, -190)
	inventory_panel.size = Vector2(460, 380)
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
	hint.text = "Materiais coletados e disponíveis para construção"
	hint.add_theme_font_size_override("font_size", 12)
	stack.add_child(hint)
	inventory_grid = GridContainer.new()
	inventory_grid.columns = 2
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
	for child in inventory_grid.get_children():
		child.queue_free()
	for i in range(BLOCK_NAMES.size()):
		var item := PanelContainer.new()
		var item_style := StyleBoxFlat.new()
		item_style.bg_color = Color(1, 1, 1, 0.055)
		item_style.set_corner_radius_all(9)
		item.add_theme_stylebox_override("panel", item_style)
		var label := Label.new()
		label.text = "  %s\\n  Quantidade: %d" % [BLOCK_NAMES[i], block_counts[i]]
		label.custom_minimum_size = Vector2(185, 48)
		label.add_theme_color_override("font_color", BLOCK_COLORS[i])
		item.add_child(label)
		inventory_grid.add_child(item)

func _load_settings() -> void:
	var config := ConfigFile.new()
	if config.load("user://mineplus_settings.cfg") == OK:
		look_sensitivity = clampf(float(config.get_value("controls", "sensitivity", 0.0025)), 0.0008, 0.008)
		configured_fov = clampf(float(config.get_value("graphics", "fov", 78.0)), 60.0, 100.0)

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
	var dialog := ConfirmationDialog.new()
	dialog.title = "BANCADA DE FABRICAÇÃO"
	dialog.dialog_text = "Receita: 2 Terra + 1 Pedra = 2 Tijolos reforçados."
	dialog.ok_button_text = "FABRICAR 2"
	dialog.confirmed.connect(_craft_bricks)
	add_child(dialog)
	dialog.popup_centered()

func _craft_bricks() -> void:
	if block_counts[1] < 2 or block_counts[2] < 1:
		status_label.text = "Faltam materiais: 2 Terra e 1 Pedra."
		return
	block_counts[1] -= 2
	block_counts[2] -= 1
	block_counts[5] += 2
	_update_hotbar()
	status_label.text = "Fabricação concluída: +2 Tijolos."

func _save_world() -> void:
	var saved_blocks: Array = []
	for cell in blocks.keys():
		saved_blocks.append([cell.x, cell.y, cell.z, int(blocks[cell].get_meta("block_type"))])
	var payload := {"version": 1, "blocks": saved_blocks, "inventory": block_counts}
	var file := FileAccess.open("user://mineplus_world.json", FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(payload))
		status_label.text = "Mundo salvo neste dispositivo."
	else:
		status_label.text = "Não foi possível salvar o mundo."

func _load_world() -> void:
	if not FileAccess.file_exists("user://mineplus_world.json"):
		return
	var file := FileAccess.open("user://mineplus_world.json", FileAccess.READ)
	if file == null:
		return
	var parsed = JSON.parse_string(file.get_as_text())
	if typeof(parsed) != TYPE_DICTIONARY or not parsed.has("blocks"):
		return
	for body in blocks.values():
		body.queue_free()
	blocks.clear()
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
	button.custom_minimum_size = Vector2(54, 54)
	button.button_down.connect(func(): Input.action_press(action))
	button.button_up.connect(func(): Input.action_release(action))
	parent.add_child(button)

func _update_hotbar() -> void:
	if hotbar_label:
		hotbar_label.text = "  1 GRAMA  2 TERRA  3 PEDRA  4 AREIA  5 MUSGO  6 TIJOLO  |  " + BLOCK_NAMES[selected_block].to_upper() + " x" + str(block_counts[selected_block])

func _physics_process(delta: float) -> void:
	if player == null:
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
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
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
			block_counts[block_type] += 1
			_remove_block(cell)
			_update_hotbar()
			_refresh_inventory()

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_save_world()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

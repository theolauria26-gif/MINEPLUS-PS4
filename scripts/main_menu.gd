extends Control

const BG := Color("#090b10")
const PANEL := Color(0.055, 0.065, 0.085, 0.92)
const ACCENT := Color("#8ee3b1")
const MUTED := Color("#a7afbd")

func _ready() -> void:
	_build_ui()

func _build_ui() -> void:
	var background := ColorRect.new()
	background.color = BG
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(background)

	var glow := ColorRect.new()
	glow.color = Color(0.10, 0.24, 0.20, 0.18)
	glow.size = Vector2(1280, 260)
	add_child(glow)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(520, 460)
	center.add_child(panel)
	var style := StyleBoxFlat.new()
	style.bg_color = PANEL
	style.border_color = Color(1, 1, 1, 0.10)
	style.set_border_width_all(1)
	style.set_corner_radius_all(22)
	style.content_margin_left = 34
	style.content_margin_right = 34
	style.content_margin_top = 30
	style.content_margin_bottom = 30
	panel.add_theme_stylebox_override("panel", style)

	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 14)
	panel.add_child(stack)

	var eyebrow := Label.new()
	eyebrow.text = "COMMUNITY EDITION  /  EARLY BUILD"
	eyebrow.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	eyebrow.add_theme_color_override("font_color", ACCENT)
	eyebrow.add_theme_font_size_override("font_size", 12)
	stack.add_child(eyebrow)

	var title := Label.new()
	title.text = "MINEPLUS"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_color_override("font_color", Color("#f4f7fb"))
	title.add_theme_font_size_override("font_size", 44)
	stack.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Seu mundo. Seu ritmo. Sua experiência."
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_color_override("font_color", MUTED)
	stack.add_child(subtitle)

	var spacer := Control.new()
	spacer.custom_minimum_size.y = 10
	stack.add_child(spacer)
	_add_menu_button(stack, "JOGAR", true, _on_play_pressed)
	_add_menu_button(stack, "PERSONALIZAR HUD", false, _on_hud_pressed)
	_add_menu_button(stack, "CONFIGURAÇÕES", false, _on_settings_pressed)

	var footer := Label.new()
	footer.text = "ANDROID  •  PC  •  PORTABILIDADE EM DESENVOLVIMENTO"
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.add_theme_color_override("font_color", Color("#687180"))
	footer.add_theme_font_size_override("font_size", 10)
	stack.add_child(footer)

func _add_menu_button(parent: VBoxContainer, caption: String, primary: bool, callback: Callable) -> void:
	var button := Button.new()
	button.text = caption
	button.custom_minimum_size = Vector2(0, 54)
	button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	button.add_theme_font_size_override("font_size", 15)
	var normal := StyleBoxFlat.new()
	normal.bg_color = ACCENT if primary else Color(1, 1, 1, 0.045)
	normal.border_color = ACCENT if primary else Color(ACCENT, 0.35)
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(13)
	button.add_theme_stylebox_override("normal", normal)
	var hover := normal.duplicate() as StyleBoxFlat
	hover.bg_color = Color("#b5f2cf") if primary else Color(1, 1, 1, 0.10)
	button.add_theme_stylebox_override("hover", hover)
	button.add_theme_color_override("font_color", Color("#101712") if primary else Color("#edf2f8"))
	button.pressed.connect(callback)
	parent.add_child(button)

func _on_play_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/world.tscn")

func _on_hud_pressed() -> void:
	_show_notice("Durante a partida, use os botões da HUD. O inventário abre pela tecla E ou pelo botão INVENTÁRIO.")

func _on_settings_pressed() -> void:
	var config := ConfigFile.new()
	config.load("user://mineplus_settings.cfg")
	var dialog := ConfirmationDialog.new()
	dialog.title = "CONFIGURAÇÕES"
	dialog.ok_button_text = "SALVAR"
	dialog.dialog_text = "Ajuste a experiência de jogo:"
	var stack := VBoxContainer.new()
	stack.custom_minimum_size = Vector2(380, 280)
	dialog.add_child(stack)
	var fov_label := Label.new()
	fov_label.text = "Campo de visão (FOV)"
	stack.add_child(fov_label)
	var fov := HSlider.new()
	fov.min_value = 60
	fov.max_value = 100
	fov.step = 1
	fov.value = float(config.get_value("graphics", "fov", 78.0))
	fov.custom_minimum_size.x = 340
	stack.add_child(fov)
	var sensitivity_label := Label.new()
	sensitivity_label.text = "Sensibilidade da câmera"
	stack.add_child(sensitivity_label)
	var sensitivity := HSlider.new()
	sensitivity.min_value = 0.0008
	sensitivity.max_value = 0.008
	sensitivity.step = 0.0002
	sensitivity.value = float(config.get_value("controls", "sensitivity", 0.0025))
	sensitivity.custom_minimum_size.x = 340
	stack.add_child(sensitivity)
	var skin_label := Label.new()
	skin_label.text = "Aparência do personagem"
	stack.add_child(skin_label)
	var skin := OptionButton.new()
	skin.add_item("Clássica")
	skin.add_item("Castanha")
	skin.add_item("Clara")
	skin.add_item("Lavanda")
	skin.select(clampi(int(config.get_value("appearance", "skin", 0)), 0, 3))
	skin.custom_minimum_size.x = 340
	stack.add_child(skin)
	var world_label := Label.new()
	world_label.text = "Slot de mundo"
	stack.add_child(world_label)
	var world_choice := OptionButton.new()
	world_choice.add_item("Mundo 1")
	world_choice.add_item("Mundo 2")
	world_choice.add_item("Mundo 3")
	world_choice.select(clampi(int(config.get_value("world", "slot", 1)) - 1, 0, 2))
	world_choice.custom_minimum_size.x = 340
	stack.add_child(world_choice)
	dialog.confirmed.connect(func():
		var saved := ConfigFile.new()
		saved.set_value("graphics", "fov", fov.value)
		saved.set_value("controls", "sensitivity", sensitivity.value)
		saved.set_value("appearance", "skin", skin.selected)
		saved.set_value("world", "slot", world_choice.selected + 1)
		var result := saved.save("user://mineplus_settings.cfg")
		if result == OK:
			_show_notice("Configurações salvas. Elas serão aplicadas ao iniciar uma partida.")
		else:
			_show_notice("Não foi possível salvar as configurações.")
	)
	add_child(dialog)
	dialog.popup_centered()

func _show_notice(message: String) -> void:
	var dialog := AcceptDialog.new()
	dialog.dialog_text = message
	add_child(dialog)
	dialog.popup_centered()

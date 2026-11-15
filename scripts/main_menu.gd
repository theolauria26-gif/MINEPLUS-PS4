extends Control

const BG := Color("#090b10")
const PANEL := Color(0.055, 0.065, 0.085, 0.88)
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
	glow.position = Vector2(0, 0)
	glow.size = Vector2(1280, 260)
	add_child(glow)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(520, 430)
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
	stack.add_theme_constant_override("separation", 16)
	panel.add_child(stack)

	var eyebrow := Label.new()
	eyebrow.text = "MINEPLUS  /  COMMUNITY EDITION"
	eyebrow.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	eyebrow.add_theme_color_override("font_color", ACCENT)
	eyebrow.add_theme_font_size_override("font_size", 13)
	stack.add_child(eyebrow)

	var title := Label.new()
	title.text = "MINEPLUS"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_color_override("font_color", Color("#f4f7fb"))
	title.add_theme_font_size_override("font_size", 42)
	stack.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Seu mundo. Seu ritmo. Sua experiência."
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_color_override("font_color", MUTED)
	stack.add_child(subtitle)

	var spacer := Control.new()
	spacer.custom_minimum_size.y = 14
	stack.add_child(spacer)

	_add_menu_button(stack, "JOGAR", true, _on_play_pressed)
	_add_menu_button(stack, "PERSONALIZAR HUD", false, _on_hud_pressed)
	_add_menu_button(stack, "CONFIGURAÇÕES", false, _on_settings_pressed)

	var footer := Label.new()
	footer.text = "PROTÓTIPO INICIAL  •  GODOT"
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.add_theme_color_override("font_color", Color("#687180"))
	footer.add_theme_font_size_override("font_size", 11)
	stack.add_child(footer)

func _add_menu_button(parent: VBoxContainer, caption: String, primary: bool, callback: Callable) -> void:
	var button := Button.new()
	button.text = caption
	button.custom_minimum_size = Vector2(0, 54)
	button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	button.add_theme_font_size_override("font_size", 15)
	var normal := StyleBoxFlat.new()
	normal.bg_color = ACCENT if primary else Color(1, 1, 1, 0.045)
	normal.border_color = Color(ACCENT, 0.45) if not primary else ACCENT
	normal.set_border_width_all(1)
	normal.set_corner_radius_all(13)
	normal.content_margin_left = 18
	normal.content_margin_right = 18
	button.add_theme_stylebox_override("normal", normal)
	var hover := normal.duplicate() as StyleBoxFlat
	hover.bg_color = Color("#b5f2cf") if primary else Color(1, 1, 1, 0.10)
	button.add_theme_stylebox_override("hover", hover)
	button.add_theme_color_override("font_color", Color("#101712") if primary else Color("#edf2f8"))
	button.pressed.connect(callback)
	parent.add_child(button)

func _on_play_pressed() -> void:
	_show_notice("A cena de jogo será conectada na próxima etapa.")

func _on_hud_pressed() -> void:
	_show_notice("Editor de HUD: próxima etapa do protótipo.")

func _on_settings_pressed() -> void:
	_show_notice("Configurações: próxima etapa do protótipo.")

func _show_notice(message: String) -> void:
	var dialog := AcceptDialog.new()
	dialog.dialog_text = message
	add_child(dialog)
	dialog.popup_centered()

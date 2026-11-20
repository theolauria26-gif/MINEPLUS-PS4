extends Control
class_name MineplusWorldLobby
## Lobby local de três slots. Nunca oferece sobrescrever um arquivo que não conseguiu validar.

const BG := Color("#090d13")
const PANEL := Color("#111923")
const CARD := Color("#17222e")
const ACCENT := Color("#8ee3b1")
const TEXT := Color("#edf3f5")
const MUTED := Color("#a7b4c0")
const SLOT_COUNT := 3
const BASE_WORLD_SEED := 4839201
const SLOT_SEED_STEP := 104729

static func save_path_for_slot(slot: int) -> String:
	var safe_slot := clampi(slot, 1, SLOT_COUNT)
	if safe_slot == 1:
		return "user://mineplus_world.json"
	return "user://mineplus_world_%d.json" % safe_slot

static func default_world_seed(slot: int) -> int:
	return BASE_WORLD_SEED + (clampi(slot, 1, SLOT_COUNT) - 1) * SLOT_SEED_STEP

static func slot_state(has_save_file: bool, payload: Dictionary) -> String:
	if not has_save_file:
		return "empty"
	return "saved" if not payload.is_empty() else "invalid"

func _ready() -> void:
	_build_ui()

func _build_ui() -> void:
	var background := ColorRect.new()
	background.color = BG
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(background)

	var glow := ColorRect.new()
	glow.color = Color(0.12, 0.31, 0.25, 0.12)
	glow.position = Vector2(0, 0)
	glow.size = Vector2(1280, 180)
	glow.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(glow)

	var center := CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)

	var viewport_size := get_viewport().get_visible_rect().size
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(minf(860.0, viewport_size.x - 24.0), minf(650.0, viewport_size.y - 24.0))
	center.add_child(panel)
	var panel_style := StyleBoxFlat.new()
	panel_style.bg_color = PANEL
	panel_style.border_color = Color(ACCENT, 0.24)
	panel_style.set_border_width_all(1)
	panel_style.set_corner_radius_all(22)
	panel_style.content_margin_left = 24
	panel_style.content_margin_right = 24
	panel_style.content_margin_top = 20
	panel_style.content_margin_bottom = 20
	panel.add_theme_stylebox_override("panel", panel_style)

	var stack := VBoxContainer.new()
	stack.add_theme_constant_override("separation", 12)
	panel.add_child(stack)

	var header := HBoxContainer.new()
	stack.add_child(header)
	var back_button := Button.new()
	back_button.text = "← VOLTAR"
	back_button.custom_minimum_size = Vector2(112, 42)
	back_button.pressed.connect(_on_back_pressed)
	header.add_child(back_button)
	var header_spacer := Control.new()
	header_spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(header_spacer)
	var local_label := Label.new()
	local_label.text = "PERFIL LOCAL  •  3 SLOTS"
	local_label.add_theme_color_override("font_color", ACCENT)
	local_label.add_theme_font_size_override("font_size", 11)
	header.add_child(local_label)

	var title := Label.new()
	title.text = "SELECIONE SEU MUNDO"
	title.add_theme_color_override("font_color", TEXT)
	title.add_theme_font_size_override("font_size", 30)
	stack.add_child(title)

	var subtitle := Label.new()
	subtitle.text = "Continue uma aventura salva ou crie um novo mundo sem substituir os outros slots."
	subtitle.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	subtitle.add_theme_color_override("font_color", MUTED)
	stack.add_child(subtitle)

	var divider := HSeparator.new()
	divider.add_theme_color_override("separator_color", Color(1, 1, 1, 0.12))
	stack.add_child(divider)

	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	stack.add_child(scroll)

	var cards := VBoxContainer.new()
	cards.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	cards.add_theme_constant_override("separation", 10)
	scroll.add_child(cards)
	for slot in range(1, SLOT_COUNT + 1):
		cards.add_child(_build_slot_card(slot))

	var footer := Label.new()
	footer.text = "OS MUNDOS SÃO SALVOS LOCALMENTE NESTE DISPOSITIVO."
	footer.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	footer.add_theme_color_override("font_color", Color("#71808d"))
	footer.add_theme_font_size_override("font_size", 10)
	stack.add_child(footer)

func _read_slot(slot: int) -> Dictionary:
	var path := save_path_for_slot(slot)
	var has_save_file := FileAccess.file_exists(path) or FileAccess.file_exists(path + ".bak")
	var payload: Dictionary = MineplusWorldSave.load_world(path) if has_save_file else {}
	return {
		"state": slot_state(has_save_file, payload),
		"path": path,
		"payload": payload
	}

func _build_slot_card(slot: int) -> PanelContainer:
	var info := _read_slot(slot)
	var state: String = info["state"]
	var payload: Dictionary = info["payload"]
	var card := PanelContainer.new()
	card.custom_minimum_size = Vector2(0, 126)
	var card_style := StyleBoxFlat.new()
	card_style.bg_color = CARD
	card_style.border_color = Color(ACCENT, 0.18) if state != "invalid" else Color("#d77979")
	card_style.set_border_width_all(1)
	card_style.set_corner_radius_all(14)
	card_style.content_margin_left = 16
	card_style.content_margin_right = 16
	card_style.content_margin_top = 12
	card_style.content_margin_bottom = 12
	card.add_theme_stylebox_override("panel", card_style)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 14)
	card.add_child(row)

	var details := VBoxContainer.new()
	details.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	details.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	details.add_theme_constant_override("separation", 5)
	row.add_child(details)

	var heading := Label.new()
	heading.text = "MUNDO %d" % slot
	heading.add_theme_color_override("font_color", TEXT)
	heading.add_theme_font_size_override("font_size", 18)
	details.add_child(heading)

	var status := Label.new()
	status.add_theme_font_size_override("font_size", 11)
	match state:
		"saved":
			status.text = "AVENTURA SALVA"
			status.add_theme_color_override("font_color", ACCENT)
		"invalid":
			status.text = "ARQUIVO NÃO VALIDADO"
			status.add_theme_color_override("font_color", Color("#ef9999"))
		_:
			status.text = "NOVO MUNDO"
			status.add_theme_color_override("font_color", Color("#9bb7d1"))
	details.add_child(status)

	var summary := Label.new()
	summary.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	summary.add_theme_color_override("font_color", MUTED)
	summary.add_theme_font_size_override("font_size", 11)
	summary.text = _slot_summary(state, payload, String(info["path"]))
	details.add_child(summary)

	var action := Button.new()
	action.custom_minimum_size = Vector2(154, 48)
	action.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	action.text = "CONTINUAR" if state == "saved" else ("INDISPONÍVEL" if state == "invalid" else "CRIAR MUNDO")
	action.disabled = state == "invalid"
	action.pressed.connect(_start_world.bind(slot))
	row.add_child(action)
	return card

func _slot_summary(state: String, payload: Dictionary, path: String) -> String:
	if state == "invalid":
		return "O arquivo existe, mas o save e o backup não passaram pela validação. Ele foi preservado; este slot não pode ser sobrescrito."
	if state == "empty":
		return "Ainda sem progresso. Seed inicial: %d. Criar aqui não altera os outros mundos." % default_world_seed(1)
	var blocks: Array = payload.get("blocks", [])
	var edits: Array = payload.get("terrain_edits", [])
	var inventory: Array = payload.get("inventory", [])
	var total_items := 0
	for amount in inventory:
		total_items += int(amount)
	var modified := FileAccess.get_modified_time(ProjectSettings.globalize_path(path))
	var date_text := Time.get_datetime_string_from_unix_time(modified, true) if modified > 0 else "data indisponível"
	return "Seed %d  •  %d blocos construídos  •  %d alterações no terreno  •  %d itens\nÚltimo save: %s" % [
		int(payload.get("world_seed", 0)), blocks.size(), edits.size(), total_items, date_text
	]

func _start_world(slot: int) -> void:
	var info := _read_slot(slot)
	if info["state"] == "invalid":
		_show_notice("Este save não passou pela validação. O arquivo foi preservado e não será sobrescrito.")
		return

	var config := ConfigFile.new()
	var load_result := config.load("user://mineplus_settings.cfg")
	if load_result != OK and load_result != ERR_FILE_NOT_FOUND:
		_show_notice("Não foi possível ler as configurações. O mundo não foi iniciado para evitar perder a seleção do slot.")
		return
	config.set_value("world", "slot", slot)
	var save_result := config.save("user://mineplus_settings.cfg")
	if save_result != OK:
		_show_notice("Não foi possível salvar a seleção do mundo. Verifique o espaço disponível no dispositivo.")
		return
	var scene_result := get_tree().change_scene_to_file("res://scenes/world.tscn")
	if scene_result != OK:
		_show_notice("Não foi possível abrir a cena do mundo. Código: %d" % scene_result)

func _on_back_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")

func _show_notice(message: String) -> void:
	var dialog := AcceptDialog.new()
	dialog.dialog_text = message
	add_child(dialog)
	dialog.popup_centered()

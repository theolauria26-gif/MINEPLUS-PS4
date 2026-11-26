extends RefCounted
class_name MineplusEquipment
## Pure equipment-selection rules shared by the world, save codec and tests.

const TOOL_NONE := ""
const TOOL_PICKAXE := "pickaxe"
const TOOL_AXE := "axe"
const MAX_DURABILITY := 40

static func normalize_selected_tool(value: Variant, pickaxe_durability: int, axe_durability: int) -> String:
	var tool := str(value)
	if tool == TOOL_PICKAXE and pickaxe_durability > 0:
		return TOOL_PICKAXE
	if tool == TOOL_AXE and axe_durability > 0:
		return TOOL_AXE
	return TOOL_NONE

static func durability_for(tool: String, pickaxe_durability: int, axe_durability: int) -> int:
	match tool:
		TOOL_PICKAXE:
			return clampi(pickaxe_durability, 0, MAX_DURABILITY)
		TOOL_AXE:
			return clampi(axe_durability, 0, MAX_DURABILITY)
	return 0

static func can_use(required_tool: String, selected_tool: String, pickaxe_durability: int, axe_durability: int) -> Dictionary:
	if required_tool.is_empty():
		return {"ok": true, "reason": ""}
	if required_tool not in [TOOL_PICKAXE, TOOL_AXE]:
		return {"ok": false, "reason": "Ferramenta necessária inválida."}
	if selected_tool != required_tool:
		var tool_name := "picareta" if required_tool == TOOL_PICKAXE else "machado"
		return {"ok": false, "reason": "Equipe a %s para quebrar este bloco." % tool_name}
	if durability_for(required_tool, pickaxe_durability, axe_durability) <= 0:
		var broken_name := "picareta" if required_tool == TOOL_PICKAXE else "machado"
		return {"ok": false, "reason": "Sua %s quebrou. Fabrique outra na bancada." % broken_name}
	return {"ok": true, "reason": ""}

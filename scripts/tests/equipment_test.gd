extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS EQUIPMENT TEST FAILED: " + description)

func _run() -> void:
	_expect(MineplusEquipment.normalize_selected_tool("pickaxe", 12, 0) == MineplusEquipment.TOOL_PICKAXE, "durable pickaxe selection survives normalization")
	_expect(MineplusEquipment.normalize_selected_tool("axe", 0, 7) == MineplusEquipment.TOOL_AXE, "durable axe selection survives normalization")
	_expect(MineplusEquipment.normalize_selected_tool("pickaxe", 0, 7).is_empty(), "broken pickaxe is never restored as equipped")
	_expect(MineplusEquipment.normalize_selected_tool("axe", 9, 0).is_empty(), "broken axe is never restored as equipped")
	_expect(MineplusEquipment.normalize_selected_tool("unknown", 9, 9).is_empty(), "unknown equipment IDs safely fall back to empty hand")
	_expect(MineplusEquipment.durability_for("pickaxe", 80, 0) == 40, "pickaxe durability is capped at its supported maximum")
	_expect(MineplusEquipment.durability_for("axe", 0, -4) == 0, "negative axe durability is clamped to zero")
	_expect(MineplusEquipment.can_use("", "", 0, 0).get("ok", false), "materials without a tool requirement remain mineable")
	_expect(not MineplusEquipment.can_use("pickaxe", "", 12, 0).get("ok", true), "having a pickaxe is not enough when it is not equipped")
	_expect(MineplusEquipment.can_use("pickaxe", "pickaxe", 12, 0).get("ok", false), "equipped durable pickaxe satisfies stone requirement")
	_expect(not MineplusEquipment.can_use("pickaxe", "axe", 12, 12).get("ok", true), "wrong equipped tool cannot mine stone")
	_expect(not MineplusEquipment.can_use("axe", "axe", 12, 0).get("ok", true), "broken equipped axe cannot mine wood")
	_expect(not MineplusEquipment.can_use("hammer", "hammer", 12, 12).get("ok", true), "unknown required tools are rejected")

	if failures == 0:
		print("MINEPLUS equipment tests: OK")
	else:
		push_error("MINEPLUS equipment tests: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

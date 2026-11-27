extends SceneTree

var failures := 0

func _initialize() -> void:
	call_deferred("_run")

func _expect(condition: bool, description: String) -> void:
	if not condition:
		failures += 1
		push_error("MINEPLUS ACTION ANIMATION TEST FAILED: " + description)

func _run() -> void:
	_expect(is_zero_approx(MineplusActionAnimation.envelope(0.0)), "action pose starts at the neutral pose")
	_expect(is_zero_approx(MineplusActionAnimation.envelope(1.0)), "action pose returns to neutral at the end")
	_expect(is_zero_approx(MineplusActionAnimation.envelope(-2.0)), "negative progress is clamped safely")
	_expect(is_zero_approx(MineplusActionAnimation.envelope(3.0)), "progress above one is clamped safely")
	_expect(is_equal_approx(MineplusActionAnimation.envelope(0.5), 1.0), "the action reaches its peak halfway through")
	var right_offset := MineplusActionAnimation.arm_offset(0.5, 1.0)
	var left_offset := MineplusActionAnimation.arm_offset(0.5, -1.0)
	_expect(right_offset.z < 0.0, "the dominant arm moves forward during the action")
	_expect(absf(left_offset.z) < absf(right_offset.z), "the supporting arm moves less than the dominant arm")
	var right_rotation := MineplusActionAnimation.arm_rotation(0.5, 1.0)
	var left_rotation := MineplusActionAnimation.arm_rotation(0.5, -1.0)
	_expect(right_rotation.x < -40.0, "the dominant arm performs a clear downward swing")
	_expect(absf(left_rotation.x) < absf(right_rotation.x), "the supporting arm has a restrained counter-motion")
	_expect(MineplusActionAnimation.arm_offset(0.0, 1.0) == Vector3.ZERO, "the action offset does not pop at its start")
	_expect(MineplusActionAnimation.arm_rotation(1.0, 1.0) == Vector3.ZERO, "the action rotation returns to neutral")

	if failures == 0:
		print("MINEPLUS action-animation tests: OK")
	else:
		push_error("MINEPLUS ACTION ANIMATION TESTS: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)

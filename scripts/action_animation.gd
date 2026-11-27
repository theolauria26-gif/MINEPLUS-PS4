extends RefCounted
class_name MineplusActionAnimation
## Small deterministic first-person action pose layered over the walking cycle.

static func envelope(progress: float) -> float:
	var normalized := clampf(progress, 0.0, 1.0)
	if normalized <= 0.0 or normalized >= 1.0:
		return 0.0
	return sin(normalized * PI)

static func arm_offset(progress: float, side: float) -> Vector3:
	var weight := 1.0 if side > 0.0 else 0.42
	var pulse := envelope(progress) * weight
	return Vector3(0.0, -0.035 * pulse, -0.18 * pulse)

static func arm_rotation(progress: float, side: float) -> Vector3:
	var weight := 1.0 if side > 0.0 else 0.42
	var pulse := envelope(progress) * weight
	return Vector3(-42.0 * pulse, 0.0, -side * 4.0 * pulse)

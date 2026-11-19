shader_type spatial;
render_mode cull_back, depth_draw_opaque;

uniform float wind_strength : hint_range(0.0, 0.12) = 0.045;
uniform float wind_speed : hint_range(0.0, 4.0) = 1.15;

void vertex() {
	// Alpha below 0.95 marks foliage; bark, stone and crystal remain rigid.
	float foliage_mask = 1.0 - step(0.95, COLOR.a);
	float height_mask = clamp((VERTEX.y - 1.0) / 6.0, 0.0, 1.0);
	float phase = TIME * wind_speed + VERTEX.x * 0.31 + VERTEX.z * 0.23;
	VERTEX.x += sin(phase) * wind_strength * foliage_mask * height_mask;
	VERTEX.z += cos(phase * 0.83) * wind_strength * 0.45 * foliage_mask * height_mask;
}

void fragment() {
	ALBEDO = COLOR.rgb;
	ROUGHNESS = 0.94;
}

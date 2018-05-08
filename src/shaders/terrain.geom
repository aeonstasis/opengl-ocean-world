R"zzz(#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
uniform vec4 light_position;
uniform vec3 camera_position;
in vec3 off[];
in vec3 norm[];
out vec4 normal;
out vec4 light_direction;
out vec4 camera_direction;
out vec4 world_position;
flat out vec3 offset;

const float kPi = 3.1415926535897932384626433832795f;

float fade(float t) {
  return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

float random(vec2 co) {
	return fract(sin(dot(co, vec2(12.9898,78.233))) * 43758.5453);
}

vec2 randUnitVec(vec2 xz) {
	float angle = random(xz) * 2 * kPi;
	return normalize(vec2(cos(angle), sin(angle)));
}

float dotGridGradient(int iu, int iv, float u, float v) {
	// Generate seeded random unit gradient vector
	vec2 unit_gradient = randUnitVec(vec2(iu, iv));

	// Compute distance vector
	float du = u - float(iu);
	float dv = v - float(iv);

	// Return dot product
	return (du * unit_gradient[0] + dv * unit_gradient[1]);
}

float perlin(float u, float v) {
	// Get coordinates of unit cell
	int u0 = int(floor(u));
	int u1 = u0 + 1;
	int v0 = int(floor(v));
	int v1 = v0 + 1;

	// Interpolation weights
	float wt_u = fade(u - float(u0));
	float wt_v = fade(v - float(v0));

	// Interpolate between grid point gradients
	float n0, n1, iu0, iu1, value;
	n0 = dotGridGradient(u0, v0, u, v);
	n1 = dotGridGradient(u1, v0, u, v);
	iu0 = mix(n0, n1, wt_u);
	n0 = dotGridGradient(u0, v1, u, v);
	n1 = dotGridGradient(u1, v1, u, v);
	iu1 = mix(n0, n1, wt_u);
	value = mix(iu0, iu1, wt_v);

	return value;
}

void emitPrimitive(vec4 position[3]) {
  int n = 0;
  for (n = 0; n < gl_in.length(); n++) {
    light_direction = vec4(normalize(light_position.xyz - position[n].xyz), 1.0f);
    camera_direction = vec4(normalize(camera_position - position[n].xyz), 1.0f);
    world_position = position[n];
    gl_Position = projection * view * model * (position[n]);
    vec3 perlin_normal = norm[n];
    perlin_normal.y += perlin(world_position.x, world_position.z) * 0.8f;
    normal = vec4(normalize(perlin_normal), 0.0f);
    EmitVertex();
  }
  EndPrimitive();
}

void main() {
  vec3 offset = off[0];
  vec4 position[3];
	position[0] = gl_in[0].gl_Position;
	position[1] = gl_in[1].gl_Position;
	position[2] = gl_in[2].gl_Position;
	emitPrimitive(position);
}
)zzz"

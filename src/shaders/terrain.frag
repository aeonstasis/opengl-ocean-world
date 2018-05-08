R"zzz(
#version 430 core
uniform vec3 center_position;
uniform vec3 camera_position;
uniform float time_of_day;
uniform bool is_raining;
in vec4 normal;
in vec4 light_direction;
in vec4 world_position;
in vec4 camera_direction;
flat in vec3 offset;
out vec4 fragment_color;

const float kPi = 3.1415926535897932384626433832795f;
const float fog_near_plane = 60.0f;
const float fog_far_plane = 70.0f;

float fade(float t) {
  return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}

vec4 calculateSkyColor() {
  if (is_raining) {
    vec4 light_gray = vec4(0.66f, 0.66f, 0.66f, 1.0f);
    vec4 dark_gray = vec4(0, 0, 0, 1.0f);
    if (time_of_day < 300.0f || time_of_day > 1140.0f) { // Before 5 AM, After 7 PM
      return dark_gray;
    } else {
      float scale = pow(abs(720.0f - time_of_day) / 420.0f, 2.0f);
      return mix(light_gray, dark_gray, scale);
    }
  } else {
    vec4 dark = vec4(0.1f, 0.1f, 0.3f, 1.0f);
    vec4 bright_blue = vec4(0.53f, 0.81f, 0.92f, 1.0f);
    if (time_of_day < 300.0f || time_of_day > 1140.0f) { // Before 5 AM, After 7 PM
      return dark;
    } else {
      float scale = pow(abs(720.0f - time_of_day) / 420.0f, 2.0f);
      return mix(bright_blue, dark, scale);
    }
  }
}

vec4 processFog(in vec3 color) {
	float fog_coefficient = (fog_far_plane - distance(world_position.xz, camera_position.xz)) / (fog_far_plane - fog_near_plane);
	fog_coefficient = clamp(fog_coefficient, 0.0f, 1.0f);
	vec4 fog_color = calculateSkyColor();
	return mix(fog_color, vec4(color, 1.0f), fog_coefficient);
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

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float multipass_noise(vec3 p) {
  float sum = 0.0f;
  float amp = 8.0f / 15.0f;
  float freq_divisor = 1.0f;
  for (int n = 0; n < 4; n++) {
    sum += noise(p * freq_divisor) * amp;//perlin(p.x * freq_divisor, p.z * freq_divisor) * amp;
    freq_divisor *= 2.0f;
    amp /= 2.0f;
  }
  return sum;
}

vec3 grass_texture() {
  float r = multipass_noise(world_position.xyz);
  r = clamp(r, 0.0f, 1.0f);
  vec3 green = vec3(0.1333f, 0.54f, 0.133f);
  vec3 dark_green = vec3(0.0f, 0.20f, 0.0f);
  return mix(green, dark_green, r);
}

void main() {
	vec3 color = vec3(0.0f, 0.0f, 0.0f);
	vec4 specular = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	float shininess = 1.0f;
  if (world_position.y > 1.0f) { // GRASS
		color = grass_texture();//vec3(0.133f, 0.54f, 0.133f);
	} else { // DIRT
    color = vec3(0.341f, 0.231f, 0.047f);
    if (world_position.y > 0.0f) {
      color = mix(grass_texture(), color, 1.0f - world_position.y);
    }
	}

	vec4 ambient = vec4(0.2f, 0.2f, 0.2f, 1.0f);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0f, 1.0f);
	vec4 spec = specular * pow(clamp(dot(reflect(-light_direction.xyz, normal.xyz), camera_direction.xyz), 0.0f, 1.0f), shininess);
	color = clamp(dot_nl * color + vec3(ambient) + vec3(spec), 0.0f, 1.0f);
  if (world_position.y < 0.0f) {
    float coeff = clamp(world_position.y / -10.0f, 0.0f, 1.0f);
    vec3 water_color = vec3(0.1f, 0.1f, 0.3f);
    color = mix(color, water_color, coeff);
  }
	fragment_color = processFog(color);
}
)zzz"

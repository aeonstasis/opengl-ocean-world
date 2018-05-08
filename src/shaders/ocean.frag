R"zzz(
#version 430 core
uniform vec3 center_position;
uniform vec3 camera_position;
uniform float time_of_day;
uniform bool is_raining;
in vec4 face_normal;
in vec4 light_direction;
in vec4 world_position;
in vec4 camera_direction;
flat in vec3 offset;
out vec4 fragment_color;

const float fog_near_plane = 60.0f;
const float fog_far_plane = 70.0f;
const float transparency_scale = 20.0f;

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
  float dist = distance(world_position.xz, camera_position.xz);
	float fog_coefficient = (fog_far_plane - dist) / (fog_far_plane - fog_near_plane);
	fog_coefficient = clamp(fog_coefficient, 0.0f, 1.0f);
	vec4 fog_color = calculateSkyColor();
  float transparency = 0.9f;
	return mix(fog_color, vec4(color, transparency), fog_coefficient);
}

void main() {
	vec3 color = vec3(0.016f, 0.0825f, 0.408f);
	vec4 specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float shininess = 20.0f;
	vec4 ambient = vec4(0.2f, 0.2f, 0.2f, 1.0f);
	float dot_nl = dot(normalize(light_direction), normalize(face_normal));
	dot_nl = clamp(dot_nl, 0.0f, 1.0f);
	vec4 spec = specular * pow(clamp(dot(reflect(-light_direction.xyz, face_normal.xyz), camera_direction.xyz), 0.0f, 1.0f), shininess);
	color = clamp(dot_nl * color + vec3(ambient) + vec3(spec), 0.0f, 1.0f);
	fragment_color = processFog(color);
}
)zzz"

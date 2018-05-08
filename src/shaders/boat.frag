R"zzz(
#version 430 core
uniform float time_of_day;
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 camera_direction;
in vec2 uv_coords;
layout(binding = 0) uniform sampler2D textureSampler;
layout(location = 0) out vec4 fragment_color;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
void main() {

  if (time_of_day < 300.0f || time_of_day > 1140.0f) { // Before 5 AM, After 7 PM
    fragment_color = vec4(0.2f, 0.2f, 0.2f, 1.0f);
    return;
  }

	vec3 texcolor = texture(textureSampler, uv_coords).xyz;
  float alpha = 1.0f;
	if (length(texcolor) == 0.0) {
    // Manually specified constants
    vec4 specular = vec4(0.1f, 0.1f, 0.1f, 1.0f);
    vec4 diffuse = vec4(0.625f, 0.32f, 0.175f, 1.0f);
    vec4 ambient = vec4(0.2f, 0.2f, 0.2f, 1.0f);
    float shininess = 1.0f;

		vec3 color = diffuse.xyz;
		float dot_nl = dot(normalize(light_direction), normalize(vertex_normal));
		dot_nl = clamp(dot_nl, 0.0, 1.0);
		vec4 spec = specular * pow(max(0.0, dot(reflect(-light_direction, vertex_normal), camera_direction)), shininess);
		color = clamp(dot_nl * color + vec3(ambient) + vec3(spec), 0.0, 1.0);
		fragment_color = vec4(color, alpha);
	} else {
		fragment_color = vec4(texcolor.rgb, alpha);
	}
}
)zzz"

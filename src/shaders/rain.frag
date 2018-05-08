R"zzz(
#version 430 core
flat in vec3 off;
out vec4 fragment_color;

void main() {
	vec3 color = vec3(0.77f, 0.88f, 0.96f);
	fragment_color = vec4(color, 0.7f);
}
)zzz"

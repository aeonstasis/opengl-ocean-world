R"zzz(
#version 430 core
in vec4 vertex_position;
in vec3 offset;
out vec3 off;

void main() {
	vec3 pos = vertex_position.xyz;
	pos.x += offset.x;
	pos.z += offset.z;
	gl_Position = vec4(pos, 1.0f);
	off = offset;
}
)zzz"

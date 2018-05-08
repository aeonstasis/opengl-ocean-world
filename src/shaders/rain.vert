R"zzz(
#version 430 core
in vec4 vertex_position;
in vec3 offset;
uniform vec3 center_position;
uniform mat4 projection;
uniform mat4 view;
flat out vec3 off;

void main() {
	vec3 pos = vertex_position.xyz + offset;
  pos.xz += center_position.xz;
	gl_Position = projection * view * vec4(pos, 1.0f);
	off = offset;
}
)zzz"

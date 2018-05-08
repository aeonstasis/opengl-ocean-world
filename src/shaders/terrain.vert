R"zzz(
#version 430 core
in vec4 vertex_position;
in vec3 offset;
in vec4 heightVec;
in vec3 norm0;
in vec3 norm1;
in vec3 norm2;
in vec3 norm3;
out vec3 off;
out vec3 norm;

void main() {
	vec4 pos = vec4(vertex_position.xyz + offset, 1.0f);
	pos.y = vertex_position.y;
	if (vertex_position.xz == vec2(0, 0)) {
		pos.y += heightVec[0];
		norm = norm0;
	}
	else if (vertex_position.xz == vec2(1.0f, 0)) {
		pos.y += heightVec[1];
		norm = norm1;
	}
	else if (vertex_position.xz == vec2(0, 1.0f)) {
		pos.y += heightVec[2];
		norm = norm2;
	} else {
		pos.y += heightVec[3];
		norm = norm3;
	}
	gl_Position = pos;
	off = offset;
}
)zzz"

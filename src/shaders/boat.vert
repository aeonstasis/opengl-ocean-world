R"zzz(
#version 430 core
uniform vec4 light_position;
uniform vec3 camera_position;
uniform vec3 center_position;
uniform vec3 prev_move;
uniform vec3 boat_pos_normal;
in vec4 vertex_position;
in vec3 normal;
in vec2 uv;
out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out int id;
out vec4 vs_camera_direction;

mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec3 rotate(vec3 v, vec3 axis, float angle) {
	mat4 m = rotationMatrix(axis, angle);
	return (m * vec4(v, 1.0)).xyz;
}

void main() {
	mat3 rot = mat3(1.0f);
	if (prev_move.xz != vec2(0.0f, 0.0f)) {
		vec2 look = normalize(prev_move.xz);
		rot[2] = vec3(look[0], 0.0f, look[1]);
		rot[0] = cross(rot[1], rot[2]);
	}
	vec3 pos = rot * vertex_position.xyz;

	// apply wave normal
	vec3 y_axis = vec3(0.0f, 1.0f, 0.0f);
	pos = rotate(pos, cross(y_axis, boat_pos_normal), -acos(dot(y_axis, boat_pos_normal)) * 0.3f);

	gl_Position = vec4(pos, 1.0f) + vec4(center_position, 0.0);
	vs_light_direction = light_position - gl_Position;
	vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_normal = vec4(normal, 0.0f);
	vs_uv = uv;
	id = 0;
}
)zzz"

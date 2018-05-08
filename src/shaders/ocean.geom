R"zzz(#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
uniform vec4 light_position;
uniform vec3 camera_position;
in vec3 off[];
in vec3 normal[];
out vec4 face_normal;
out vec4 light_direction;
out vec4 camera_direction;
out vec4 world_position;
flat out vec3 offset;

void emitPrimitive(vec4 position[3]) {
  int n = 0;
  for (n = 0; n < gl_in.length(); n++) {
    light_direction = vec4(normalize(light_position.xyz - position[n].xyz), 1.0f);
    camera_direction = vec4(normalize(camera_position - position[n].xyz), 1.0f);
    world_position = position[n];
    gl_Position = projection * view * model * (position[n]);
    face_normal = vec4(normal[n], 0.0f);
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

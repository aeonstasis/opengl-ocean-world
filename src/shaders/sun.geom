R"zzz(#version 400 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
uniform vec4 light_position;
uniform mat4 view;
void main()
{
  int n = 0;
  for (n = 0; n < gl_in.length(); n++) {
    gl_Position = projection * view * vec4(gl_in[n].gl_Position.xyz + light_position.xyz, 1.0f);
    EmitVertex();
  }
  EndPrimitive();
}
)zzz"

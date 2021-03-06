R"zzz(#version 430 core
uniform mat4 inverse_projection_view;
smooth out vec3 eye_direction;
void main()
{
  vec2 position = vec2((gl_VertexID & 2) >> 1, 1 - (gl_VertexID & 1)) * 2.0 - 1.0;
  vec4 front = inverse_projection_view * vec4(position, -1.0, 1.0);
  vec4 back = inverse_projection_view * vec4(position, 1.0, 1.0);
  eye_direction = normalize(back.xyz / back.w - front.xyz / front.w);
  gl_Position = vec4(position, -1.0, 1.0);
}
)zzz"

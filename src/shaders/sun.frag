R"zzz(#version 400 core
uniform float time_of_day;
out vec4 fragment_color;
void main()
{
  // TODO: maybe scale the color based on time of day?
  fragment_color = vec4(0.98, 0.83, 0.25, 1.0);
}
)zzz"

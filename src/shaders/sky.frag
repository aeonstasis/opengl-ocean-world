R"zzz(#version 430 core
smooth in vec3 eye_direction;
uniform bool is_raining;
uniform float time_of_day;
out vec4 fragment_color;

vec4 calculateSkyColor() {
  if (is_raining) {
    vec4 light_gray = vec4(0.66f, 0.66f, 0.66f, 1.0f);
    vec4 dark_gray = vec4(0, 0, 0, 1.0f);
    if (time_of_day < 300.0f || time_of_day > 1140.0f) { // Before 5 AM, After 7 PM
      return dark_gray;
    } else {
      float scale = pow(abs(720.0f - time_of_day) / 420.0f, 2.0f);
      return mix(light_gray, dark_gray, scale);
    }
  } else {
    vec4 dark = vec4(0.1f, 0.1f, 0.3f, 1.0f);
    vec4 bright_blue = vec4(0.53f, 0.81f, 0.92f, 1.0f);
    if (time_of_day < 300.0f || time_of_day > 1140.0f) { // Before 5 AM, After 7 PM
      return dark;
    } else {
      float scale = pow(abs(720.0f - time_of_day) / 420.0f, 2.0f);
      return mix(bright_blue, dark, scale);
    }
  }
}

void main()
{
  fragment_color = calculateSkyColor();
}
)zzz"

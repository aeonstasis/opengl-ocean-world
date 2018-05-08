#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <random>

namespace perlin {
float kPi = 3.1415926535897932384626433832795f;
float kBlockSize = 1.0f;
float kMaxHeight = 50.0f;

float fade(float t) {
  return 6 * glm::pow(t, 5) - 15 * glm::pow(t, 4) + 10 * glm::pow(t, 3);
}

float random(const glm::vec2 &co) {
  return glm::fract(glm::sin(glm::dot(co, glm::vec2(12.9898, 78.233))) *
                    43758.5453);
}

glm::vec2 randUnitVec(const glm::vec2 &xz) {
  float angle = random(xz) * 2 * kPi;
  return glm::normalize(glm::vec2(glm::cos(angle), glm::sin(angle)));
}

float dotGridGradient(int ix, int iz, float x, float z) {
  // Generate seeded random unit gradient vector
  auto unit_gradient = randUnitVec(glm::vec2(ix, iz));

  // Compute distance vector
  float dx = x - static_cast<float>(ix);
  float dz = z - static_cast<float>(iz);

  // Return dot product
  return (dx * unit_gradient[0] + dz * unit_gradient[1]);
}

// Compute Perlin noise for the given coordinates
float perlin(float x, float z) {
  // Get coordinates of unit cell
  int x0 = std::floor(x);
  int x1 = x0 + 1;
  int z0 = std::floor(z);
  int z1 = z0 + 1;

  // Interpolation weights
  float wt_x = fade(x - static_cast<float>(x0));
  float wt_z = fade(z - static_cast<float>(z0));

  // Interpolate between grid point gradients
  float n0, n1, ix0, ix1, value;
  n0 = dotGridGradient(x0, z0, x, z);
  n1 = dotGridGradient(x1, z0, x, z);
  ix0 = glm::mix(n0, n1, wt_x);
  n0 = dotGridGradient(x0, z1, x, z);
  n1 = dotGridGradient(x1, z1, x, z);
  ix1 = glm::mix(n0, n1, wt_x);
  value = glm::mix(ix0, ix1, wt_z);

  return value;
}

float multipass_noise(float x, float z) {
  float sum = 0.0f;
  float amp = 8.0f / 15.0f;
  float freq_divisor = 1.0f;
  for (int n = 0; n < 3; n++) {
    sum += perlin(x * freq_divisor, z * freq_divisor) * amp;
    freq_divisor *= 2.0f;
    amp /= 2.0f;
  }
  return sum;
}

float getHeight(float x, float z) {
  return multipass_noise(x / kBlockSize / 20.0f, z / kBlockSize / 20.0f) *
             kMaxHeight * kBlockSize -
         8.0f;
}

} /* namespace perlin */

#pragma once

#include "render_pass.h"
#include <chrono>

class RainRender {
public:
  RainRender(size_t rows, size_t cols, std::vector<ShaderUniform> uniforms);

  void update(bool draw);

private:
  void move_particles(float time_delta);

  std::vector<glm::vec3> rain_points_;
  std::unique_ptr<RenderPass> rain_pass_;
  std::chrono::high_resolution_clock::time_point prev =
      std::chrono::high_resolution_clock::now();
};

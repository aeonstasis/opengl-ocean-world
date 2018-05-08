#pragma once

#include "render_pass.h"
#include <chrono>
#include <glm/glm.hpp>
#include <memory>

class TerrainRender {
public:
  TerrainRender(size_t rows, size_t cols, std::vector<ShaderUniform> uniforms);
  void renderVisible(glm::vec3 eye);
  bool isPositionLegal(const glm::vec3 &loc);
  float getWaveHeight(const glm::vec3 &loc);
  void setStartTime(std::chrono::high_resolution_clock::time_point t) {
    start_time_ = t;
  }
  float getTime() {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(end - start_time_).count();
  }

  glm::vec3 getWaveNormal(const glm::vec3 &loc);
  void toggle_storm(bool is_raining);

private:
  void updateInstanceOffsets(int x, int z);
  void updateWaveParams();

  std::chrono::high_resolution_clock::time_point start_time_;
  size_t ticks_;
  size_t rows_;
  size_t cols_;
  int cached_x_;
  int cached_z_;
  std::unique_ptr<RenderPass> terrain_pass_;
  std::unique_ptr<RenderPass> ocean_pass_;
  std::vector<glm::vec3> instanceOffsets_;
  std::vector<glm::vec3> sortedOffsets_;
  std::vector<glm::vec4> heightVec_;
  std::vector<glm::vec3> norm0_;
  std::vector<glm::vec3> norm1_;
  std::vector<glm::vec3> norm2_;
  std::vector<glm::vec3> norm3_;
};

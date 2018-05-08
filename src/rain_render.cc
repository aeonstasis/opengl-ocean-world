#include "rain_render.h"
#include <GL/glew.h>
#include <cstdio>
#include <iostream>
#include <random>

const float kRainSpeed = 50.0f;
const float kRainDropLength = 0.5f;
const float kResetHeight = 100.0f;
const float kResetThreshold = 0.0f;

const char *rain_vertex_shader =
#include "shaders/rain.vert"
    ;

const char *rain_fragment_shader =
#include "shaders/rain.frag"
    ;

const std::array<glm::vec4, 2> line_vertices = {
    {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, kRainDropLength, 0.0f, 1.0f}}};

const std::array<glm::uvec2, 1> line_index = {{{0, 1}}};

RainRender::RainRender(size_t rows, size_t cols,
                       std::vector<ShaderUniform> uniforms)
    : rain_points_(rows * cols) {
  size_t index = 0;
  for (size_t i = 0; i < rows; i++) {
    for (size_t j = 0; j < cols; j++) {
      auto chance = fmodf(rand(), 100.0f);
      if (chance < 50.0f) {
        continue;
      }
      float height =
          fmodf(rand(), kResetHeight - kResetThreshold) + kResetThreshold;
      rain_points_[index++] = {float(i) - float(rows) / 2, height,
                               float(j) - float(cols) / 2};
    }
  }
  rain_points_.resize(index);

  auto rain_pass_input = RenderDataInput{};
  rain_pass_input.assign(0, "vertex_position", line_vertices.data(),
                         line_vertices.size(), 4, GL_FLOAT);
  rain_pass_input.assign(1, "offset", rain_points_.data(), rain_points_.size(),
                         3, GL_FLOAT, true);
  rain_pass_input.assignIndex(line_index.data(), line_index.size(), 2);
  auto rain_shaders = std::vector<const char *>{
      {rain_vertex_shader, nullptr, rain_fragment_shader}};
  auto output = std::vector<const char *>{{"fragment_color"}};
  this->rain_pass_ = std::make_unique<RenderPass>(
      -1, rain_pass_input, rain_shaders, uniforms, output);
}

void RainRender::move_particles(float time_delta) {
  for (size_t i = 0; i < rain_points_.size(); i++) {
    auto point = rain_points_.at(i);
    if (point.y < kResetThreshold) {
      point.y += kResetHeight - kResetThreshold;
    } else {
      point.y -= kRainSpeed * time_delta;
    }
    rain_points_[i] = point;
  }
  rain_pass_->updateVBO(1, rain_points_.data(), rain_points_.size());
}

void RainRender::update(bool draw) {
  auto now = std::chrono::high_resolution_clock::now();
  move_particles(std::chrono::duration<float>(now - prev).count());
  prev = now;
  if (draw) {
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    rain_pass_->setup();
    glDrawElementsInstanced(GL_LINES, 2, GL_UNSIGNED_INT, 0,
                            rain_points_.size());
  }
}

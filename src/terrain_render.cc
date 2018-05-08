#include "terrain_render.h"

#include "perlin.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <random>

const char *terrain_vertex_shader =
#include "shaders/terrain.vert"
    ;

const char *terrain_geometry_shader =
#include "shaders/terrain.geom"
    ;

const char *terrain_fragment_shader =
#include "shaders/terrain.frag"
    ;

const char *ocean_vertex_shader =
#include "shaders/ocean.vert"
    ;

const char *ocean_geometry_shader =
#include "shaders/ocean.geom"
    ;

const char *ocean_fragment_shader =
#include "shaders/ocean.frag"
    ;

const char *ocean_tcs_shader =
#include "shaders/ocean.tcs"
    ;

const char *ocean_tes_shader =
#include "shaders/ocean.tes"
    ;

using std::array;
using std::vector;

constexpr float BLOCK_SIZE = 1.0f;
constexpr int UPDATE_STEP = 5;
constexpr double kPi = 3.141592653589793;
constexpr double kG = 9.8000001;

// Defines a basic unit cube in 3-space
const array<glm::vec4, 4> cube_vertices = {
    {{0.0f, 0.0f, 0.0f, 1.0f},
     {0.0f, 0.0f, BLOCK_SIZE, 1.0f},
     {BLOCK_SIZE, 0.0f, 0.0f, 1.0f},
     {BLOCK_SIZE, 0.0f, BLOCK_SIZE, 1.0f}}};

const array<glm::uvec3, 2> cube_faces = {{{2, 0, 1}, {1, 3, 2}}};

// Wave simulation parameters
constexpr int kNumWaves = 10;
float gMedianWave = 30.0f; /* wavelengths sampled based on this average wave */
float gMedianAmp = 0.10f;  /* amplitudes sampled based on this average amp */
float gSteepness = 0.3f;   /* tunable *sharpness* of wave in [0, 1] */
glm::vec3 gMedianDir = {0.5f, 0.1f, 0.5f}; /* sample directions relative */
float kAngleRange = kPi / 3;               /* sample directions within  range */
array<float, kNumWaves> gAmp{};
array<float, kNumWaves> gFreq{};
array<float, kNumWaves> gPhi{};
array<glm::vec3, kNumWaves> gDir{};

TerrainRender::TerrainRender(size_t rows, size_t cols,
                             std::vector<ShaderUniform> uniforms)
    : ticks_(0), rows_(rows), cols_(cols), cached_x_(0), cached_z_(0),
      instanceOffsets_(rows * cols), sortedOffsets_(rows * cols),
      heightVec_(rows * cols), norm0_(rows * cols), norm1_(rows * cols),
      norm2_(rows * cols), norm3_(rows * cols) {

  // WAVES
  // Binders
  auto param_binder = [](int loc, const void *data) {
    glUniform1fv(loc, kNumWaves, (const GLfloat *)data);
  };
  auto dir_binder = [](int loc, const void *data) {
    glUniform3fv(loc, kNumWaves, (const GLfloat *)data);
  };
  auto int_binder = [](int loc, const void *data) {
    glUniform1iv(loc, 1, (const GLint *)data);
  };
  auto float_binder = [](int loc, const void *data) {
    glUniform1fv(loc, 1, (const GLfloat *)data);
  };

  // Data
  auto amp_data = []() -> const void * { return gAmp.data(); };
  auto freq_data = []() -> const void * { return gFreq.data(); };
  auto phi_data = []() -> const void * { return gPhi.data(); };
  auto dir_data = []() -> const void * { return gDir.data(); };
  auto steepness_data = []() -> const void * { return &gSteepness; };
  auto num_waves_data = []() -> const void * { return &kNumWaves; };

  // Uniforms
  uniforms.push_back({"amp", param_binder, amp_data});
  uniforms.push_back({"freq", param_binder, freq_data});
  uniforms.push_back({"phi", param_binder, phi_data});
  uniforms.push_back({"dir", dir_binder, dir_data});
  uniforms.push_back({"steepness", float_binder, steepness_data});
  uniforms.push_back({"num_waves", int_binder, num_waves_data});

  auto terrain_pass_input = RenderDataInput{};
  terrain_pass_input.assign(0, "vertex_position", cube_vertices.data(),
                            cube_vertices.size(), 4, GL_FLOAT);

  // Set up offsets for each instanced cube
  updateInstanceOffsets(0, 0);
  terrain_pass_input.assign(1, "offset", instanceOffsets_.data(),
                            instanceOffsets_.size(), 3, GL_FLOAT, true);
  terrain_pass_input.assign(2, "heightVec", heightVec_.data(),
                            heightVec_.size(), 4, GL_FLOAT, true);
  terrain_pass_input.assign(3, "norm0", norm0_.data(), norm0_.size(), 3,
                            GL_FLOAT, true);
  terrain_pass_input.assign(4, "norm1", norm1_.data(), norm1_.size(), 3,
                            GL_FLOAT, true);
  terrain_pass_input.assign(5, "norm2", norm2_.data(), norm2_.size(), 3,
                            GL_FLOAT, true);
  terrain_pass_input.assign(6, "norm3", norm3_.data(), norm3_.size(), 3,
                            GL_FLOAT, true);
  terrain_pass_input.assignIndex(cube_faces.data(), cube_faces.size(), 3);

  // Shader-related construct arguments for RenderPass
  auto terrain_shaders =
      vector<const char *>{{terrain_vertex_shader, terrain_geometry_shader,
                            terrain_fragment_shader}};
  auto output = vector<const char *>{{"fragment_color"}};
  this->terrain_pass_ = std::make_unique<RenderPass>(
      -1, terrain_pass_input, terrain_shaders, uniforms, output);

  // WATER
  auto ocean_pass_input = RenderDataInput{};
  ocean_pass_input.assign(0, "vertex_position", cube_vertices.data(),
                          cube_vertices.size(), 4, GL_FLOAT);
  ocean_pass_input.assign(1, "offset", sortedOffsets_.data(),
                          sortedOffsets_.size(), 3, GL_FLOAT, true);
  ocean_pass_input.assignIndex(cube_faces.data(), cube_faces.size(), 3);
  auto ocean_shaders = vector<const char *>{
      {ocean_vertex_shader, ocean_geometry_shader, ocean_fragment_shader,
       ocean_tcs_shader, ocean_tes_shader}};
  this->ocean_pass_ = std::make_unique<RenderPass>(
      -1, ocean_pass_input, ocean_shaders, uniforms, output);

  // Initialize wave parameters
  updateWaveParams();
}

void TerrainRender::renderVisible(glm::vec3 eye) {
  int x_coord = std::floor(eye.x / BLOCK_SIZE);
  int z_coord = std::floor(eye.z / BLOCK_SIZE);

  // Update wave parameters every ~3sec
  ticks_++;
  if (ticks_ % 180 == 0) {
    // updateWaveParams();
  }

  // Only update instanceOffsets_ if eye changes
  if (x_coord / UPDATE_STEP != cached_x_ / UPDATE_STEP ||
      z_coord / UPDATE_STEP != cached_z_ / UPDATE_STEP) {
    updateInstanceOffsets(x_coord, z_coord);

    terrain_pass_->updateVBO(1, instanceOffsets_.data(),
                             instanceOffsets_.size());
    terrain_pass_->updateVBO(2, heightVec_.data(), heightVec_.size());
    terrain_pass_->updateVBO(3, norm0_.data(), norm0_.size());
    terrain_pass_->updateVBO(4, norm1_.data(), norm1_.size());
    terrain_pass_->updateVBO(5, norm2_.data(), norm2_.size());
    terrain_pass_->updateVBO(6, norm3_.data(), norm3_.size());
    ocean_pass_->updateVBO(1, sortedOffsets_.data(), sortedOffsets_.size());
  }

  // Draw each cube, instanced
  terrain_pass_->setup();
  glDrawElementsInstanced(GL_TRIANGLES, cube_faces.size() * 3, GL_UNSIGNED_INT,
                          0, instanceOffsets_.size());
  ocean_pass_->setup();
  glDrawElementsInstanced(GL_PATCHES, cube_faces.size() * 3, GL_UNSIGNED_INT, 0,
                          sortedOffsets_.size());
}

/**
 * Vary the ocean parameters with time to achieve a dynamic wave simulation.
 * This also allows us to achieve "stormy" weather vs. "sunny" weather.
 */
void TerrainRender::updateWaveParams() {
  static std::mt19937 engine(std::random_device{}());

  double now = getTime();

  // TODO: Update median wave and amplitude

  // Resample wavelengths to generate new frequencies
  auto lengths = array<float, kNumWaves>{};
  auto freq_dist = std::uniform_real_distribution<float>(gMedianWave / 2.0,
                                                         gMedianWave * 2.0);
  std::generate(lengths.begin(), lengths.end(),
                [&freq_dist]() { return freq_dist(engine); });
  std::transform(lengths.begin(), lengths.end(), gFreq.begin(),
                 [](const auto &wavelength) {
                   return std::sqrt(kG * 2 * kPi / wavelength);
                 });

  // Derive amplitudes based on wavelengths (constant ratio)
  auto amp_dist =
      std::uniform_real_distribution<float>(gMedianAmp / 2.0, gMedianAmp * 2.0);
  std::generate(gAmp.begin(), gAmp.end(),
                [&amp_dist]() { return amp_dist(engine); });
  /*
    std::transform(lengths.begin(), lengths.end(), gAmp.begin(),
                   [](const auto &wavelength) {
                     return
                     // return gMedianAmp / gMedianWave * wavelength;
                   });*/

  // Resample direction vectors
  auto dir_dist =
      std::uniform_real_distribution<float>(-kAngleRange / 2, kAngleRange / 2);
  std::generate(gDir.begin(), gDir.end(), [&dir_dist]() {
    return glm::rotateY(gMedianDir, dir_dist(engine));
  });

  // Update phase values
  auto phase_dist = std::uniform_real_distribution<float>(-kPi, kPi);
  std::generate(gPhi.begin(), gPhi.end(),
                [&phase_dist]() { return phase_dist(engine); });
}

void TerrainRender::updateInstanceOffsets(int x, int z) {
  int index = 0;
  instanceOffsets_.resize(rows_ * cols_);
  for (size_t i = 0; i < rows_; i++) {
    for (size_t j = 0; j < cols_; j++) {
      float newX = ((float)i - (float)(rows_ / 2) + (float)x) * BLOCK_SIZE;
      float newZ = ((float)j - (float)(cols_ / 2) + (float)z) * BLOCK_SIZE;
      float perlin = perlin::getHeight(newX, newZ);
      instanceOffsets_[index++] = {newX, perlin, newZ};
    }
  }
  index = 0;
  heightVec_.resize(rows_ * cols_);
  std::vector<glm::vec3> normals(rows_ * cols_);
  for (size_t i = 0; i < rows_; i++) {
    for (size_t j = 0; j < cols_; j++) {
      float botLeft = instanceOffsets_[index].y;
      glm::vec4 localHeights = glm::vec4{botLeft};
      if (i < rows_ - 1) { // Up
        localHeights[1] = instanceOffsets_[index + cols_].y;
      }
      if (j < cols_ - 1) { // Right
        localHeights[2] = instanceOffsets_[index + 1].y;
      }
      if (i < rows_ - 1 && j < cols_ - 1) { // Diag
        localHeights[3] = instanceOffsets_[index + cols_ + 1].y;
      }
      normals[index] = -glm::normalize(
          glm::cross(glm::vec3{1.0f, localHeights[1] - botLeft, 0.0f},
                     glm::vec3{0.0f, localHeights[2] - botLeft, 1.0f}));
      heightVec_[index] = localHeights;
      index++;
    }
  }

  index = 0;
  for (size_t i = 0; i < rows_; i++) {
    for (size_t j = 0; j < cols_; j++) {
      norm0_[index] = normals[index];
      norm1_[index] = (i < rows_ - 1) ? normals[index + cols_] : normals[index];
      norm2_[index] = (j < cols_ - 1) ? normals[index + 1] : normals[index];
      norm3_[index] = (i < rows_ - 1 && j < cols_ - 1)
                          ? normals[index + cols_ + 1]
                          : normals[index];
      index++;
    }
  }

  sortedOffsets_ = instanceOffsets_;
  glm::vec2 center_pos = {x, z};
  std::sort(sortedOffsets_.begin(), sortedOffsets_.end(),
            [&center_pos](const glm::vec3 &a, const glm::vec3 &b) {
              return glm::distance(center_pos, glm::vec2{a.x, a.z}) <
                     glm::distance(center_pos, glm::vec2{b.x, b.z});
            });

  cached_x_ = x;
  cached_z_ = z;
}

bool TerrainRender::isPositionLegal(const glm::vec3 &loc) {
  // Check that player (if treated as a line) lies above terrain
  int i = int(int(floor(loc.x)) - cached_x_ + rows_ / 2);
  int j = int(int(floor(loc.z)) - cached_z_ + cols_ / 2);
  float currentBlockHeight = instanceOffsets_[i * cols_ + j].y;
  if (currentBlockHeight >= 0.0f && loc.y < currentBlockHeight) {
    return false;
  }

  // Check for collisions with left, right, front, back
  float x_center = std::floor(loc.x);
  float z_center = std::floor(loc.z);
  auto loc_left = glm::vec3{x_center - 1.0f, 0, z_center};
  auto loc_right = glm::vec3{x_center + 1.0f, 0, z_center};
  auto loc_front = glm::vec3{x_center, 0, z_center + 1.0f};
  auto loc_back = glm::vec3{x_center, 0, z_center - 1.0f};
  auto loc_left_front = glm::vec3{x_center - 1.0f, 0, z_center + 1.0f};
  auto loc_right_front = glm::vec3{x_center + 1.0f, 0, z_center + 1.0f};
  auto loc_left_back = glm::vec3{x_center - 1.0f, 0, z_center - 1.0f};
  auto loc_right_back = glm::vec3{x_center + 1.0f, 0, z_center - 1.0f};
  auto neighbors = vector<glm::vec3>{
      loc_left,       loc_right,       loc_front,     loc_back,
      loc_left_front, loc_right_front, loc_left_back, loc_right_back};

  for (const auto &neighbor : neighbors) {
    // Check circle-rectangle intersection (xz-plane)
    auto closest = glm::vec2{glm::clamp(loc.x, neighbor.x, neighbor.x + 1.0f),
                             glm::clamp(loc.z, neighbor.z, neighbor.z + 1.0f)};
    float distance = glm::distance(closest, {loc.x, loc.z});
    if (distance < 0.25f) {
      // Check heights (y)
      int i = int(floor(neighbor.x)) - cached_x_ + rows_ / 2;
      int j = int(floor(neighbor.z)) - cached_z_ + cols_ / 2;
      auto block_height = instanceOffsets_[i * cols_ + j].y;
      if (block_height >= 0.0f && loc.y < block_height) {
        return false;
      }
    }
  }

  return true;
}

float TerrainRender::getWaveHeight(const glm::vec3 &loc) {
  // Calculate wave height at a given location
  float waveHeight = 0.0f;
  // Naive sum of sines
  /*for (size_t i = 0; i < gAmp.size(); i++) {
    if (gAmp.at(i) == 0.0f) {
      break;
    }
    waveHeight +=
        gAmp.at(i) * glm::sin(glm::dot(glm::vec2(gDir.at(i).x, gDir.at(i).z),
                                       glm::vec2(loc.x, loc.z)) *
                                  gFreq.at(i) +
                              getTime() * gPhi.at(i));
  }*/
  glm::vec2 loc_xz = {loc.x, loc.z};
  for (size_t i = 0; i < kNumWaves; i++) {
    glm::vec2 dir_xz = {gDir.at(i).x, gDir.at(i).z};
    waveHeight += gAmp.at(i) * glm::sin(glm::dot(gFreq.at(i) * dir_xz, loc_xz) +
                                        gPhi.at(i) * getTime());
  }
  return waveHeight + 0.1875f;
}

glm::vec3 TerrainRender::getWaveNormal(const glm::vec3 &loc) {
  glm::vec3 pos = loc;
  pos.y = getWaveHeight(loc);
  glm::vec3 norm = {0.0f, 1.0f, 0.0f};
  for (size_t i = 0; i < kNumWaves; i++) {
    float q = gSteepness / (gFreq.at(i) * gAmp.at(i) * kNumWaves);
    float WA = gFreq.at(i) * gAmp.at(i);
    float S = glm::sin(glm::dot(gFreq.at(i) * gDir.at(i), pos) +
                       gPhi.at(i) * getTime());
    float C = glm::cos(glm::dot(gFreq.at(i) * gDir.at(i), pos) +
                       gPhi.at(i) * getTime());
    glm::vec3 temp{0.0f};
    temp.x = -(gDir.at(i).x * WA * C);
    temp.z = -(gDir.at(i).z * WA * C);
    temp.y = -(q * WA * S);
    norm += temp;
  }
  return glm::normalize(norm);
}

void TerrainRender::toggle_storm(bool is_raining) {
  if (is_raining) {
    kAngleRange = kPi / 2;
    gMedianWave = 150.0f;
    gMedianAmp = 0.5f;
    gSteepness = 0.2f;
    updateWaveParams();
  } else {
    kAngleRange = kPi / 3;
    gMedianWave = 30.0f;
    gMedianAmp = 0.10f;
    gSteepness = 0.3f;
    updateWaveParams();
  }
}

#ifndef SKINNING_GUI_H
#define SKINNING_GUI_H

#include "terrain_render.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <vector>

struct Mesh;

/*
 * Hint: call glUniformMatrix4fv on thest pointers
 */
struct MatrixPointers {
  const float *projection, *model, *view, *inv_proj_view;
};

class GUI {
public:
  GUI(GLFWwindow *);
  ~GUI();

  void keyCallback(int key, int scancode, int action, int mods);
  void mousePosCallback(double mouse_x, double mouse_y);
  void mouseButtonCallback(int button, int action, int mods);
  void mouseScrollCallback(double dx, double dy);
  void updateMatrices();
  MatrixPointers getMatrixPointers() const;

  static void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mods);
  static void MousePosCallback(GLFWwindow *window, double mouse_x,
                               double mouse_y);
  static void MouseButtonCallback(GLFWwindow *window, int button, int action,
                                  int mods);
  static void MouseScrollCallback(GLFWwindow *window, double dx, double dy);

  const glm::vec3 &getCenter() const { return center_; }
  const glm::vec3 &getCamera() const { return eye_; }
  const float *getLightPositionPtr() const { return &light_position_[0]; }

  void updatePosition();
  glm::vec3 getMoveVec(const glm::vec3 &input);
  TerrainRender *terrainRender = nullptr;
  const float kMaxTimeOfDay = 1440.0f;
  void incrementTimeOfDay(float f) {
    time_of_day_ = fmodf(time_of_day_ + f, kMaxTimeOfDay);
  }
  float &getTimeOfDay() { return time_of_day_; }
  int &isRaining() { return raining_; }

  glm::vec3 &getPreviousMoveVec() { return previous_move_; }

private:
  GLFWwindow *window_;

  // Dimension state
  int window_width_, window_height_;

  bool drag_state_ = false;
  bool fps_mode_ = false;
  int current_button_ = -1;
  float roll_speed_ = M_PI / 64.0f;
  float last_x_ = 0.0f;
  float last_y_ = 0.0f;
  float current_x_ = 0.0f;
  float current_y_ = 0.0f;
  float camera_distance_ = 10.0;
  float pan_speed_ = 0.1f;
  float rotation_speed_ = 0.02f;
  float zoom_speed_ = 0.1f;
  float aspect_;

  glm::vec3 eye_ = glm::vec3(0.0f, 10.0f, camera_distance_);
  glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 look_ = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 tangent_ = glm::cross(look_, up_);
  glm::vec3 center_ = eye_ - camera_distance_ * look_;
  glm::mat3 orientation_ = glm::mat3(tangent_, up_, look_);
  glm::vec4 light_position_;
  float y_velocity_ = 0.0f;
  bool gravity_enabled_ = true;

  glm::mat4 view_matrix_ = glm::lookAt(eye_, center_, up_);
  glm::mat4 projection_matrix_;
  glm::mat4 model_matrix_ = glm::mat4(1.0f);
  glm::mat4 inv_proj_matrix_ = glm::inverse(projection_matrix_ * view_matrix_);
  std::unordered_map<char, bool> key_pressed_;

  bool captureWASDUPDOWN(int key, int action);
  float time_of_day_ = 9 * 60.0f;
  // each unit represents minutes of the day (so 24 * 60 = 1 day)
  int raining_ = false;
  glm::vec3 previous_move_{0.0f};
};

#endif

#include "gui.h"
#include "config.h"
#include <chrono>
#include <debuggl.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <jpegio.h>
#include <vector>

using std::vector;

// start ffmpeg telling it to expect raw rgba 720p-60hz frames
// -i - tells it to read frames from stdin

GUI::GUI(GLFWwindow *window) : window_(window) {
  glfwSetWindowUserPointer(window_, this);
  glfwSetKeyCallback(window_, KeyCallback);
  glfwSetCursorPosCallback(window_, MousePosCallback);
  glfwSetMouseButtonCallback(window_, MouseButtonCallback);
  glfwSetScrollCallback(window_, MouseScrollCallback);

  glfwGetWindowSize(window_, &window_width_, &window_height_);
  float aspect_ = static_cast<float>(window_width_) / window_height_;
  projection_matrix_ =
      glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
  key_pressed_['W'] = false;
  key_pressed_['A'] = false;
  key_pressed_['S'] = false;
  key_pressed_['D'] = false;
  key_pressed_['u'] = false;
  key_pressed_['d'] = false;
}

GUI::~GUI() {}

void GUI::keyCallback(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window_, GL_TRUE);
    return;
  }

  // Save screenshot
  if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
    // Read pixels from the framebuffer
    auto num_bytes = window_width_ * window_height_ * 3;
    auto pixels = vector<unsigned char>(num_bytes);
    glReadPixels(0, 0, window_width_, window_height_, GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());

    // Write image out to file
    SaveJPEG("screenshot.jpg", window_width_, window_height_, pixels.data());
    std::cout << "Screenshot written out to \"screenshot.jpg\"" << std::endl;
    return;
  }

  // Toggle gravity
  if (key == GLFW_KEY_F && (mods & GLFW_MOD_CONTROL) &&
      action == GLFW_RELEASE) {
    gravity_enabled_ = !gravity_enabled_;
    std::cout << "Gravity is: " << (gravity_enabled_ ? "on" : "off")
              << std::endl;
  }

  if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL) &&
      action == GLFW_RELEASE) {
    // Control + S
    // Pass
  }

  if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
    // Left/Right Arrow Keys
    // Pass
  } else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
    fps_mode_ = !fps_mode_;
  } else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_RELEASE) {
    // [
    // Pass
  } else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_RELEASE) {
    // ]
    // Pass
  } else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
    // T
    // Pass
  }

  if (key == GLFW_KEY_EQUAL && action == GLFW_RELEASE) {
    incrementTimeOfDay(60);
  }

  if (key == GLFW_KEY_1 && action == GLFW_RELEASE) {
    raining_ = !raining_;
    terrainRender->toggle_storm(raining_);
  }

  if (mods == 0 && captureWASDUPDOWN(key, action))
    return;
}

void GUI::mousePosCallback(double mouse_x, double mouse_y) {
  last_x_ = current_x_;
  last_y_ = current_y_;
  current_x_ = mouse_x;
  current_y_ = window_height_ - mouse_y;
  float delta_x = current_x_ - last_x_;
  float delta_y = current_y_ - last_y_;
  if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
    return;
  glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
  glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
  glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
  glm::uvec4 viewport = glm::uvec4(0, 0, window_width_, window_height_);

  // Calculate rotation matrix
  auto temp_orientation = orientation_;
  temp_orientation[1] = glm::vec3{0.0, 1.0, 0.0};
  glm::vec3 axis =
      glm::normalize(temp_orientation *
                     glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f));
  orientation_ =
      glm::mat3(glm::rotate(rotation_speed_, axis) * glm::mat4(orientation_));

  // Set camera instance variables
  tangent_ = glm::column(orientation_, 0);
  up_ = glm::column(orientation_, 1);
  look_ = glm::column(orientation_, 2);
}

void GUI::mouseButtonCallback(int button, int action, int mods) {
  // Mouse press
  // Pass
}

void GUI::mouseScrollCallback(double dx, double dy) {
  // Mouse scroll
  // Pass
}

void GUI::updateMatrices() {
  // Compute our view, and projection matrices.
  if (fps_mode_) {
    center_ = eye_ + camera_distance_ * look_;
    view_matrix_ = glm::lookAt(eye_, center_, up_);
  } else {
    auto stabilized_center = center_;
    stabilized_center.y = 0.0f;
    eye_ = stabilized_center - camera_distance_ * look_;
    view_matrix_ = glm::lookAt(eye_, stabilized_center, up_);
  }

  light_position_ = glm::vec4(eye_, 1.0f);

  aspect_ = static_cast<float>(window_width_) / window_height_;
  projection_matrix_ =
      glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
  model_matrix_ = glm::mat4(1.0f);
  inv_proj_matrix_ = glm::inverse(projection_matrix_ * view_matrix_);
}

MatrixPointers GUI::getMatrixPointers() const {
  MatrixPointers ret;
  ret.projection = &projection_matrix_[0][0];
  ret.model = &model_matrix_[0][0];
  ret.view = &view_matrix_[0][0];
  ret.inv_proj_view = &inv_proj_matrix_[0][0];
  return ret;
}

void GUI::updatePosition() {
  if (fps_mode_) {
    if (gravity_enabled_) {
      // Effects of gravity
      y_velocity_ -= 0.008f;
      auto new_pos = eye_ + glm::vec3{0, y_velocity_, 0};

      // Check for ground collision
      if (!terrainRender->isPositionLegal(new_pos)) {
        y_velocity_ = 0.0f;
      } else {
        eye_ = new_pos;
      }
    }
    if (key_pressed_['W']) {
      glm::vec3 move_vec = getMoveVec(zoom_speed_ * look_);
      if (terrainRender->isPositionLegal(eye_ + move_vec)) {
        eye_ += move_vec;
      }
    }
    if (key_pressed_['S']) {
      glm::vec3 move_vec = getMoveVec(zoom_speed_ * look_);
      if (terrainRender->isPositionLegal(eye_ - move_vec)) {
        eye_ -= move_vec;
      }
    }
    if (key_pressed_['A']) {
      glm::vec3 move_vec = getMoveVec(pan_speed_ * tangent_);
      if (terrainRender->isPositionLegal(eye_ - move_vec)) {
        eye_ -= move_vec;
      }
    }
    if (key_pressed_['D']) {
      glm::vec3 move_vec = getMoveVec(pan_speed_ * tangent_);
      if (terrainRender->isPositionLegal(eye_ + move_vec)) {
        eye_ += move_vec;
      }
    }
    if (key_pressed_['u'] && !gravity_enabled_) {
      if (terrainRender->isPositionLegal(eye_ + pan_speed_ * up_)) {
        eye_ += pan_speed_ * up_;
      }
    }
    if (key_pressed_['d'] && !gravity_enabled_) {
      if (terrainRender->isPositionLegal(eye_ - pan_speed_ * up_)) {
        eye_ -= pan_speed_ * up_;
      }
    }
  } else { // Center focused
    glm::vec3 move_vec{0.0f};
    if (gravity_enabled_) {
      // Effects of gravity
      y_velocity_ -= 0.008f;

      // Check for ground collision
      if (terrainRender->isPositionLegal(center_ +
                                         glm::vec3{0, y_velocity_, 0})) {
        move_vec = glm::vec3{0, y_velocity_, 0};
      } else {
        y_velocity_ = 0.0f;
      }
    }

    if (key_pressed_['W']) {
      move_vec += getMoveVec(zoom_speed_ * look_);
    }
    if (key_pressed_['S']) {
      move_vec -= getMoveVec(zoom_speed_ * look_);
    }
    if (key_pressed_['A']) {
      move_vec -= getMoveVec(pan_speed_ * tangent_);
    }
    if (key_pressed_['D']) {
      move_vec += getMoveVec(pan_speed_ * tangent_);
    }
    auto new_center = center_ + move_vec;
    if (terrainRender->isPositionLegal(new_center)) {
      center_ += move_vec;
    }
    float waveHeight = terrainRender->getWaveHeight(new_center);
    if (center_.y < waveHeight) {
      center_.y = waveHeight;
      y_velocity_ = 0.0f;
    }
    if (move_vec.x != 0 || move_vec.z != 0) {
      previous_move_ = move_vec;
    }
  }
}

glm::vec3 GUI::getMoveVec(const glm::vec3 &input) {
  if (gravity_enabled_) {
    return glm::normalize(glm::vec3{input.x, 0, input.z}) * 0.2f;
  } else {
    return input;
  }
}

bool GUI::captureWASDUPDOWN(int key, int action) {
  if (fps_mode_) {
    // When under effect of gravity, only y-axis movement via jumping
    if (gravity_enabled_) {
      if (key == GLFW_KEY_SPACE && action == GLFW_PRESS &&
          abs(y_velocity_) < 1e-7) {
        // Jumping behavior in Minecraft
        y_velocity_ += 0.2;
      }
    }
  }

  // Regular move forward/backward + strafe left/right
  if (key == GLFW_KEY_W) {
    if (action == GLFW_PRESS) {
      key_pressed_['W'] = true;
    }
    if (action == GLFW_RELEASE) {
      key_pressed_['W'] = false;
    }
    return true;
  } else if (key == GLFW_KEY_S) {
    if (action == GLFW_PRESS) {
      key_pressed_['S'] = true;
    }
    if (action == GLFW_RELEASE) {
      key_pressed_['S'] = false;
    }
    return true;
  } else if (key == GLFW_KEY_A) {
    if (action == GLFW_PRESS) {
      key_pressed_['A'] = true;
    }
    if (action == GLFW_RELEASE) {
      key_pressed_['A'] = false;
    }
    return true;
  } else if (key == GLFW_KEY_D) {
    if (action == GLFW_PRESS) {
      key_pressed_['D'] = true;
    }
    if (action == GLFW_RELEASE) {
      key_pressed_['D'] = false;
    }
    return true;
  }

  // Allow arbitrary y-axis (up/down) movement when gravity is disabled
  if (key == GLFW_KEY_DOWN) {
    if (action == GLFW_PRESS) {
      key_pressed_['d'] = true;
    }
    if (action == GLFW_RELEASE) {
      key_pressed_['d'] = false;
    }
    return true;
  } else if (key == GLFW_KEY_UP) {
    if (action == GLFW_PRESS) {
      key_pressed_['u'] = true;
    }
    if (action == GLFW_RELEASE) {
      key_pressed_['u'] = false;
    }
    return true;
  }

  return false;
}

// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                      int mods) {
  GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
  gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow *window, double mouse_x, double mouse_y) {
  GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
  gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow *window, int button, int action,
                              int mods) {
  GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
  gui->mouseButtonCallback(button, action, mods);
}

void GUI::MouseScrollCallback(GLFWwindow *window, double dx, double dy) {
  GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
  gui->mouseScrollCallback(dx, dy);
}

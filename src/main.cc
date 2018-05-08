#include <GL/glew.h>
#include <dirent.h>

#include "config.h"
#include "gui.h"
#include "procedure_geometry.h"
#include "rain_render.h"
#include "render_pass.h"
#include "terrain_render.h"
#include "util.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <debuggl.h>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

int window_width = 1280;
int window_height = 720;
int height_map_rows = 150;
int height_map_cols = 150;
const std::string window_title = "Sea of Thieves";
const float SUN_RADIUS = 100.0f;

const std::vector<glm::vec4> SUN_VERTICES = {
    {0.000f, 0.000f, 1.000f, 0.0f},   {0.894f, 0.000f, 0.447f, 0.0f},
    {0.276f, 0.851f, 0.447f, 0.0f},   {-0.724f, 0.526f, 0.447f, 0.0f},
    {-0.724f, -0.526f, 0.447f, 0.0f}, {0.276f, -0.851f, 0.447f, 0.0f},
    {0.724f, 0.526f, -0.447f, 0.0f},  {-0.276f, 0.851f, -0.447f, 0.0f},
    {-0.894f, 0.000f, -0.447f, 0.0f}, {-0.276f, -0.851f, -0.447f, 0.0f},
    {0.724f, -0.526f, -0.447f, 0.0f}, {0.000f, 0.000f, -1.000f, 0.0f}};

const std::vector<glm::uvec3> SUN_FACES = {
    {2, 1, 0},  {3, 2, 0},  {4, 3, 0},  {5, 4, 0},   {1, 5, 0},
    {11, 6, 7}, {11, 7, 8}, {11, 8, 9}, {11, 9, 10}, {11, 10, 6},
    {1, 2, 6},  {2, 3, 7},  {3, 4, 8},  {4, 5, 9},   {5, 1, 10},
    {2, 7, 6},  {3, 8, 7},  {4, 9, 8},  {5, 10, 9},  {1, 6, 10}};

const char *vertex_shader =
#include "shaders/default.vert"
    ;

const char *boat_vertex_shader =
#include "shaders/boat.vert"
    ;

const char *boat_geometry_shader =
#include "shaders/boat.geom"
    ;

const char *boat_fragment_shader =
#include "shaders/boat.frag"
    ;

const char *sky_vertex_shader =
#include "shaders/sky.vert"
    ;

const char *geometry_shader =
#include "shaders/default.geom"
    ;

const char *fragment_shader =
#include "shaders/default.frag"
    ;

const char *sky_fragment_shader =
#include "shaders/sky.frag"
    ;

const char *sun_vertex_shader =
#include "shaders/sun.vert"
    ;

const char *sun_geometry_shader =
#include "shaders/sun.geom"
    ;

const char *sun_fragment_shader =
#include "shaders/sun.frag"
    ;

const char *sun_tcs_shader =
#include "shaders/sun.tcs"
    ;

const char *sun_tes_shader =
#include "shaders/sun.tes"
    ;

void ErrorCallback(int error, const char *description) {
  std::cerr << "GLFW Error: " << description << "\n";
}

GLFWwindow *init_glefw() {
  if (!glfwInit())
    exit(EXIT_FAILURE);
  glfwSetErrorCallback(ErrorCallback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Disable resizing, for simplicity
  glfwWindowHint(GLFW_SAMPLES, 4);
  auto ret = glfwCreateWindow(window_width, window_height, window_title.data(),
                              nullptr, nullptr);
  CHECK_SUCCESS(ret != nullptr);
  glfwSetInputMode(ret, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwMakeContextCurrent(ret);
  glewExperimental = GL_TRUE;
  CHECK_SUCCESS(glewInit() == GLEW_OK);
  glGetError(); // clear GLEW's error for it
  glfwSwapInterval(1);
  const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte *version = glGetString(GL_VERSION);   // version as a string
  std::cout << "Renderer: " << renderer << "\n";
  std::cout << "OpenGL version supported:" << version << "\n";

  return ret;
}

int main(int argc, char *argv[]) {
  auto start = std::chrono::high_resolution_clock::now();
  GLFWwindow *window = init_glefw();
  GUI gui(window);

  glm::vec4 light_position = glm::vec4(0.0f, SUN_RADIUS, 0.0f, 1.0f);
  MatrixPointers mats;

  // Define MatrixPointers here for lambda to capture
  /*
   * In the following we are going to define several lambda functions to bind
   * Uniforms.
   *
   * Introduction about lambda functions:
   *      http://en.cppreference.com/w/cpp/language/lambda
   *      http://www.stroustrup.com/C++11FAQ.html#lambda
   */
  /*
   * The following lambda functions are defined to bind uniforms
   */
  auto matrix_binder = [](int loc, const void *data) {
    glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat *)data);
  };
  auto vector_binder = [](int loc, const void *data) {
    glUniform4fv(loc, 1, (const GLfloat *)data);
  };
  auto vector3_binder = [](int loc, const void *data) {
    glUniform3fv(loc, 1, (const GLfloat *)data);
  };
  auto float_binder = [](int loc, const void *data) {
    glUniform1fv(loc, 1, (const GLfloat *)data);
  };
  auto int_binder = [](int loc, const void *data) {
    glUniform1iv(loc, 1, (const GLint *)data);
  };

  /*
   * The lambda functions below are used to retrieve data
   */
  auto std_model_data = [&mats]() -> const void * {
    return mats.model;
  }; // This returns point to model matrix
  auto std_view_data = [&mats]() -> const void * { return mats.view; };
  auto std_camera_data = [&gui]() -> const void * {
    return &gui.getCamera()[0];
  };
  auto std_center_data = [&gui]() -> const void * {
    return &gui.getCenter()[0];
  };
  auto std_proj_data = [&mats]() -> const void * { return mats.projection; };
  auto inv_proj_data = [&mats]() -> const void * { return mats.inv_proj_view; };
  auto std_light_data = [&light_position]() -> const void * {
    return &light_position[0];
  };
  float time_from_start;
  auto prev = start;
  auto std_time_data = [&start, &time_from_start, &prev,
                        &gui]() -> const void * {
    auto now = std::chrono::high_resolution_clock::now();
    time_from_start = std::chrono::duration<float>(now - start).count();
    float since_last = std::chrono::duration<float>(now - prev).count();
    gui.incrementTimeOfDay(since_last);
    prev = now;
    return &time_from_start;
  };
  auto std_time_of_day_data = [&gui]() -> const void * {
    return &gui.getTimeOfDay();
  };
  auto prev_move_data = [&gui]() -> const void * {
    return &gui.getPreviousMoveVec();
  };
  glm::vec3 boat_pos_normal{0.0f};
  auto boat_pos_normal_data = [&boat_pos_normal]() -> const void * {
    return &boat_pos_normal;
  };
  auto is_raining_data = [&gui]() -> const void * { return &gui.isRaining(); };

  ShaderUniform std_model = {"model", matrix_binder, std_model_data};
  ShaderUniform std_view = {"view", matrix_binder, std_view_data};
  ShaderUniform inv_proj_view = {"inverse_projection_view", matrix_binder,
                                 inv_proj_data};
  ShaderUniform std_camera = {"camera_position", vector3_binder,
                              std_camera_data};
  ShaderUniform std_proj = {"projection", matrix_binder, std_proj_data};
  ShaderUniform std_light = {"light_position", vector_binder, std_light_data};
  ShaderUniform std_center = {"center_position", vector3_binder,
                              std_center_data};
  ShaderUniform std_time = {"time", float_binder, std_time_data};
  ShaderUniform std_time_of_day = {"time_of_day", float_binder,
                                   std_time_of_day_data};
  ShaderUniform std_prev_move = {"prev_move", vector3_binder, prev_move_data};
  ShaderUniform std_boat_pos_normal = {"boat_pos_normal", vector3_binder,
                                       boat_pos_normal_data};
  ShaderUniform std_is_raining = {"is_raining", int_binder, is_raining_data};

  //
  // Boat render pass
  //
  auto boat_mesh = util::LoadObj("../assets/rowboat.obj");
  auto boat_pass_input = RenderDataInput{};
  boat_pass_input.assign(0, "vertex_position", boat_mesh.vertices.data(),
                         boat_mesh.vertices.size(), 4, GL_FLOAT);
  boat_pass_input.assign(1, "normal", boat_mesh.normals.data(),
                         boat_mesh.vertices.size(), 3, GL_FLOAT);
  boat_pass_input.assignIndex(boat_mesh.vertex_indices.data(),
                              boat_mesh.vertex_indices.size(), 3);
  RenderPass boat_pass(
      -1, boat_pass_input,
      {boat_vertex_shader, boat_geometry_shader, boat_fragment_shader},
      {std_model, std_light, std_center, std_view, std_proj, std_prev_move,
       std_boat_pos_normal, std_time_of_day},
      {"fragment_color"});

  //
  // Sun render pass
  //
  auto sun_pass_input = RenderDataInput{};
  sun_pass_input.assign(0, "vertex_position", SUN_VERTICES.data(),
                        SUN_VERTICES.size(), 4, GL_FLOAT);
  sun_pass_input.assignIndex(SUN_FACES.data(), SUN_FACES.size(), 3);
  RenderPass sun_pass(-1, sun_pass_input,
                      {sun_vertex_shader, sun_geometry_shader,
                       sun_fragment_shader, sun_tcs_shader, sun_tes_shader},
                      {std_view, std_proj, std_light, std_time_of_day},
                      {"fragment_color"});

  //
  // Skybox render pass
  //
  RenderPass sky_pass(
      -1, RenderDataInput{}, {sky_vertex_shader, nullptr, sky_fragment_shader},
      {inv_proj_view, std_time_of_day, std_is_raining}, {"fragment_color"});

  //
  // Terrain render pass
  //
  TerrainRender terrainRender(height_map_rows, height_map_cols,
                              {std_model, std_view, std_proj, std_light,
                               std_camera, std_center, std_time,
                               std_time_of_day, std_is_raining});
  terrainRender.setStartTime(start);
  gui.terrainRender = &terrainRender;

  //
  // Rain render pass
  //
  RainRender rainRender(
      height_map_rows, height_map_cols,
      {std_view, std_proj, std_light, std_camera, std_center});

  bool draw_terrain = true;
  double previousTime = glfwGetTime();
  int frameCount = 0;
  while (!glfwWindowShouldClose(window)) {
    boat_pos_normal = terrainRender.getWaveNormal(gui.getCenter());
    // FPS Counter
    double currentTime = glfwGetTime();
    frameCount++;
    if (currentTime - previousTime >= 1.0) {
      // Display the frame count here any way you want.
      std::cout << "FPS: " << frameCount << std::endl;
      frameCount = 0;
      previousTime = currentTime;
    }

    // Calculating light_position
    float time_of_day = gui.getTimeOfDay();
    float angle = (time_of_day / gui.kMaxTimeOfDay) * 2 * M_PI;
    glm::vec4 light_vec_from_center =
        SUN_RADIUS * glm::vec4{0.0f, -cos(angle), sin(angle), 0.0f};
    light_position = light_vec_from_center + glm::vec4(gui.getCenter(), 1.0f);

    // Setup some basic window stuff.
    glfwGetFramebufferSize(window, &window_width, &window_height);
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);

    gui.updatePosition();
    gui.updateMatrices();
    mats = gui.getMatrixPointers();

    // Draw sky
    sky_pass.setup();
    glDepthMask(false);
    CHECK_GL_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    glDepthMask(true);

    // Draw terrain
    if (draw_terrain) {
      terrainRender.renderVisible(gui.getCamera());
    }

    // Draw boat
    boat_pass.setup();
    CHECK_GL_ERROR(glDrawElements(
        GL_TRIANGLES, boat_mesh.vertex_indices.size() * 3, GL_UNSIGNED_INT, 0));

    // Draw sun
    sun_pass.setup();
    CHECK_GL_ERROR(
        glDrawElements(GL_PATCHES, SUN_FACES.size() * 3, GL_UNSIGNED_INT, 0));

    // Draw rain
    rainRender.update(gui.isRaining());

    // Poll and swap.
    glfwPollEvents();
    glfwSwapBuffers(window);
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

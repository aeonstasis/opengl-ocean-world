#pragma once

#include <algorithm>
#include <array>
#include <debuggl.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <jpegio.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace util {

struct Mesh {
  std::vector<glm::vec4> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::uvec3> vertex_indices;
};

std::vector<std::string> split(const std::string &str, char delim) {
  std::stringstream ss{str};
  std::string item;
  std::vector<std::string> tokens;
  while (getline(ss, item, delim)) {
    tokens.push_back(item);
  }
  return tokens;
}

// Load geometry from OBJ file
Mesh LoadObj(const std::string &filename) {
  auto mesh = Mesh{};
  std::cout << "Reading mesh from OBJ file" << std::endl;

  // Helper objects
  std::ifstream file{filename};
  if (!file.is_open()) {
    throw std::invalid_argument("Failed to open file: " + filename);
  }
  auto line = std::string{};
  auto type = std::string{};
  auto vertex = glm::vec4{0.0, 0.0, 0.0, 1.0};
  auto normals = std::vector<glm::vec3>{};
  auto normal = glm::vec3{0.0, 0.0, 0.0};
  auto vertex_index = glm::uvec3{0, 0, 0};
  auto normal_index = glm::uvec3{0, 0, 0};

  // Parse each line
  while (std::getline(file, line)) {
    auto stream = std::istringstream{line};
    stream >> type;
    if (type == "v") {
      stream >> vertex.x >> vertex.y >> vertex.z;
      mesh.vertices.push_back(vertex);
    } else if (type == "f") {
      if (mesh.normals.size() == 0) {
        mesh.normals.resize(mesh.vertices.size());
      }
      // Each entry specifies both vertex index and normal index
      for (int i = 0; i <= 2; i++) {
        std::string temp;
        stream >> temp;
        auto parts = split(temp, '/');
        vertex_index[i] = std::stoi(parts.at(0)) - 1;
        mesh.normals[vertex_index[i]] = normals.at(std::stoi(parts.at(2)) - 1);
        // normal_index[i] = std::stoi(parts.at(2)) - 1;
      }
      mesh.vertex_indices.push_back(vertex_index);
      // mesh.normals.push_back(normals.at(normal_index[0]));
    } else if (type == "vn") {
      stream >> normal.x >> normal.y >> normal.z;
      normals.push_back(normal);
    } else if (type == "#") {
      // Skip comment
      continue;
    } else {
      std::cout << "Skipping unknown type: " << type << std::endl;
    }
  }

  // Print out some stats
  std::cout << mesh.vertices.size() << " vertices, " << mesh.normals.size()
            << " normals, " << mesh.vertex_indices.size() << " faces"
            << std::endl;

  std::cout << "Done reading mesh" << std::endl;
  return mesh;
}

std::array<std::unique_ptr<Image>, 6>
loadSkyboxImages(std::array<std::string, 6> paths) {
  auto images = std::array<std::unique_ptr<Image>, 6>{};
  for (size_t i = 0; i < 6; i++) {
    auto image = std::unique_ptr<Image>{};
    if ((image = LoadJPEG(paths[i])) == nullptr) {
      std::cout << "Failed to load: " << paths[i] << std::endl;
    }
    images[i] = std::move(image);
  }
  return images;
}

void createCubemap(GLuint programID, std::array<std::string, 6> paths) {
  auto skybox = loadSkyboxImages(paths);
  std::cout << "Cubemap loaded!" << std::endl;
  GLuint texture = 0;
  CHECK_GL_ERROR(glGenTextures(1, &texture));
  CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, texture));
  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                                 GL_CLAMP_TO_EDGE));
  CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                                 GL_CLAMP_TO_EDGE));
  CHECK_GL_ERROR(
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  CHECK_GL_ERROR(
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  for (size_t i = 0; i < 6; i++) {
    CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                                skybox[i]->width, skybox[i]->height, 0, GL_RGB,
                                GL_UNSIGNED_BYTE,
                                static_cast<void *>(skybox[i]->bytes.data())));
  }
  CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
  GLuint location = 0;
  CHECK_GL_ERROR(location = glGetUniformLocation(programID, "skybox"));
  CHECK_GL_ERROR(glProgramUniform1i(programID, location, 1));
  CHECK_GL_ERROR(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
  CHECK_GL_ERROR(glBindTextureUnit(1, texture));
}
}

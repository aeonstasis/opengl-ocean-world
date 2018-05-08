#ifndef PROCEDURE_GEOMETRY_H
#define PROCEDURE_GEOMETRY_H

#include <glm/glm.hpp>
#include <vector>

struct LineMesh;

void create_floor(std::vector<glm::vec4> &floor_vertices,
                  std::vector<glm::uvec3> &floor_faces);

#endif

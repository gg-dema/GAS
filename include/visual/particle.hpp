#pragma once

#include <glm/glm.hpp>

namespace gas::visual {

struct Particle2D {
    glm::vec2 position = {0.0, 0.0};
    glm::vec3 color = {1.0, 0.0, 0.0};
    float size = 30.0;
    float alpha = 1.0;

    Particle2D(const glm::vec2& pos_,
               const glm::vec3& color_,
               float size_,
               float alpha_)
        : position(pos_),
          color(color_),
          size(size_),
          alpha(alpha_){};
};

struct Particle3D {
    glm::vec3 position = {0.0, 0.0, 0.0};
    glm::vec3 color = {1.0, 0.0, 0.0};
    float size = 5.0;
    float alpha = 1.0;

    Particle3D(glm::vec3 pos_, glm::vec3 color_, float size_, float alpha_)
        : position(pos_),
          color(color_),
          size(size_),
          alpha(alpha_){};
};

}  // namespace gas::visual
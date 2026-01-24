#pragma once

// clang-format off
#include <glad/glad.h>   // must come before GLFW --> modern GL
#include <GLFW/glfw3.h>  // Needed for GLFWwindow --> address the window, context, event polling
// clang-format on
#include <vector>

#include "visual/particle.hpp"

namespace gas::visual {

class RenderingEngine {
   public:
    GLFWwindow* window;
    GLuint VAO, VBO;
    GLuint shaderProgram;
    GLint U_MVP = -1;

    RenderingEngine();
    ~RenderingEngine();
    void step(const std::vector<Particle2D>& particles);

   private:
    void setup_buffers();
    void setup_shader();
    void setup_projection_matrix(int w, int h);
};
}  // namespace gas::visual
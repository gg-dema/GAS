// clang-format off
#include <glad/glad.h>   // must come before GLFW --> modern GL ? 
#include <GLFW/glfw3.h>  // Needed for GLFWwindow --> address the window, context, event polling

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // really wondering what the hell this does
#include <glm/gtc/type_ptr.hpp>
// clang-format on

extern int WIDTH;
extern int HEIGHT;

#include "visual/rendering_engine.hpp"

#include <fstream>
#include <string>

#include "config.hpp"

// **************************
// Callback function
// **************************

// void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
//     glViewport(0, 0, width, height);
// }

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}
// **************************

namespace gas::visual {

RenderingEngine::RenderingEngine() {
    if (!glfwInit()) {
        throw std::runtime_error("no glfw init");
    }
    // set modern standard for openGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "particles", nullptr, nullptr);
    // the 2 nullptr are for "monitor" and "share" :
    // monitor for take the ptr of the full scree
    // share is a pointer for share resources between window
    glfwMakeContextCurrent(window);

    // DARK TRICK MAGIC: attack a void pointer to window, (the void ptr will be
    // "this")
    glfwSetWindowUserPointer(window, this);
    auto callback_size = [](GLFWwindow* win, int w, int h) {
        glViewport(0, 0, w, h);
        auto* rendering_engine =
            static_cast<RenderingEngine*>(glfwGetWindowUserPointer(win));
        rendering_engine->setup_projection_matrix(w, h);
    };

    glfwSetFramebufferSizeCallback(window, callback_size);

    // openGL funct are not available --> nor glGenBuffers or
    // glBindVertex Array.
    //  so, GLAD ask at runtime the pointer for each function
    // glad load proc : ptr to a function that loads openGL functs
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    // INDeED, THE BLUE FUNCTION ARE LATER RESPECT gladLoadGLLoader,
    // NOTE this function are dynamically load
    glEnable(GL_PROGRAM_POINT_SIZE);
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);  // Black background
    setup_buffers();
    setup_shader();

    glUseProgram(shaderProgram);
    U_MVP = glGetUniformLocation(shaderProgram, "U_MVP");
    setup_projection_matrix(WIDTH, HEIGHT);
}

RenderingEngine::~RenderingEngine() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void RenderingEngine::setup_buffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // specify which vao are using now?
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // allocate max size particle
    glBufferData(GL_ARRAY_BUFFER,
                 gas::config::MAX_PARTICLE * sizeof(Particle2D),
                 nullptr,  // data will be loaded later on
                 GL_DYNAMIC_DRAW);

    // position: vec2 with offset 0:
    glVertexAttribPointer(0,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Particle2D),
                          (void*)offsetof(Particle2D, position));
    glEnableVertexAttribArray(0);

    // color: offset of position
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Particle2D),
                          (void*)offsetof(Particle2D, color));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2,
                          1,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Particle2D),
                          (void*)offsetof(Particle2D, size));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3,
                          1,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Particle2D),
                          (void*)offsetof(Particle2D, alpha));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RenderingEngine::setup_shader() {
    // load shader source
    const char* vertex;
    const char* fragment;
    std::string source_code;

    // load vertex shader code
    std::ifstream vertexFile{"../asset/shaders/vertex_blackHole.glsl"};
    source_code = {std::istreambuf_iterator<char>(vertexFile),
                   std::istreambuf_iterator<char>()};
    vertex = source_code.c_str();
    // compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex, nullptr);
    glCompileShader(vertexShader);

    // load fragment code
    std::ifstream fragmentFile{"../asset/shaders/fragment_blackHole.glsl"};
    source_code = {std::istreambuf_iterator<char>(fragmentFile),
                   std::istreambuf_iterator<char>()};
    fragment = source_code.c_str();
    // compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment, nullptr);
    glCompileShader(fragmentShader);

    // create full program
    // ------------------
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // delete single shader, not needed anymore
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void RenderingEngine::setup_projection_matrix(int w, int h) {
    // address the different ratio between the physical world and the real world
    float aspect_ratio = float(w) / float(h);
    float world_W_dim = 1500.0F;
    float world_H_dim = world_W_dim / aspect_ratio;

    glm::mat4 proj = glm::ortho(-0.5F * world_W_dim,
                                +0.5F * world_W_dim,
                                -0.5F * world_H_dim,
                                +0.5F * world_H_dim,
                                -1.0F,
                                1.0F);
    // re-load the shaders and bind the new matrix
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(U_MVP, 1, GL_FALSE, glm::value_ptr(proj));
}

void RenderingEngine::step(const std::vector<Particle2D>& particles) {
    processInput(this->window);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    particles.size() * sizeof(Particle2D),
                    particles.data());
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(shaderProgram);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, particles.size());

    glfwSwapBuffers(window);
    glfwPollEvents();
}

}  // namespace gas::visual

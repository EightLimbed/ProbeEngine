#include "glad/glad.h"
#include <GLFW/glfw3.h>

// returns window pointer
GLFWwindow *createWindow(int width, int height, char *name) {
  if (!glfwInit()) {
    return NULL;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window = glfwCreateWindow(width, height, name, NULL, NULL);
  if (!window) {
    glfwTerminate();
    return NULL;
  }

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  // glfwSwapInterval(1); // no VSync

  return window;
}

// initializes buffer at ID
void createSSBO(GLuint ID, size_t size, int index) {
  glGenBuffers(1, &ID);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
  glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ID);
}
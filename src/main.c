#include "glad/glad.h"
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>

int main() {
  // creates a window
  player p;
  initializePlayer(&p);
  checkPlayer(&p);
  GLFWwindow *window = createWindow(800, 600, "Window");

  // render loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
  }

  // end
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
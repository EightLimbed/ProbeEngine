#include "glad/glad.h"
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>

int main() {
  // creates a window
  GLFWwindow *window = createWindow(800, 600, "Window");

  // creates player
  player p;
  {vec3 pos = {0.0,0.0,0.0};
  vec3 dir = {0.0,0.0,1.0};
  initializePlayer(&p, pos, dir, 100.0, window);}

  // render loop
  while (!glfwWindowShouldClose(window)) {
    // handles player inputs
    playerInputs(&p,1.0);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // poll events and draw screen
    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  // end
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
#include "glad/glad.h"
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>
#include <Engine/shader.h>

int main() {
  // creates a window
  GLFWwindow *window = createWindow(800, 600, "Window");

  // creates player
  player p;
  {vec3 pos = {0.0,0.0,0.0};
  vec3 dir = {0.0,0.0,1.0};
  initializePlayer(&p, pos, dir, 100.0, window);}

  // loads shaders
  GLuint ScreenID;
  {
    GLuint vID;
    GLuint fID ;
    shaderCompile(&vID, GL_VERTEX_SHADER, "shaders/4.3.screenquad.vert");
    shaderCompile(&fID, GL_FRAGMENT_SHADER , "shaders/4.3.screen.frag");
    ScreenID = linkShaders(vID, fID);
  }

  // bind vertex arrays (very important)
  GLuint vao;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  // render loop
  while (!glfwWindowShouldClose(window)) {
    // handles player inputs
    playerInputs(&p,1.0);

    //glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(ScreenID);

    // draw triangles
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // poll events and draw screen
    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  // end
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
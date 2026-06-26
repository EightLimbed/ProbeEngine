#include "glad/glad.h"
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>
#include <Engine/shader.h>

int screenHeight = 800;
int screenWidth = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

int main() {
  // creates a window
  GLFWwindow *window = createWindow(800, 600, "Window");
  // temp
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  // hide mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    processInput(window);

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

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        //glfwSetCursorPos(window, 0.0,0.0);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwFocusWindow(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    printf("Screen resized to: (%d, %d).\n", width, height);
    screenHeight = width;
    screenWidth = height;
    //updateSettings(); // updates settings based on new values.

    // make sure the viewport matches the new window dimensions; note that width and height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height); // resize viewport
}
#include "glad/glad.h"
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>
#include <Engine/shader.h>

// screen
int screenWidth = 800;
int screenHeight = 600;

// shaders
GLuint screenTex; // screen texture
GLuint ScreenID;
GLuint MarcherID;

// functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void updateSettings();

int main() {
  // creates a window
  GLFWwindow *window = createWindow(800, 600, "Window");
  // temp
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  // hide mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // creates player
  player player;
  {vec3 pos = {0.0,0.0,0.0};
  vec3 dir = {0.0,0.0,1.0};
  initializePlayer(&player, pos, dir, 100.0, 0.005, window);}

  // loads shaders
  shaderCompile(&MarcherID, GL_COMPUTE_SHADER, "shaders/4.3.raymarcher.comp");
  MarcherID = linkComputeShader(MarcherID);

  { // screen shader
    GLuint vID;
    GLuint fID ;
    shaderCompile(&vID, GL_VERTEX_SHADER, "shaders/4.3.screenquad.vert");
    shaderCompile(&fID, GL_FRAGMENT_SHADER , "shaders/4.3.screen.frag");
    ScreenID = linkShaders(vID, fID);
  }
  
  updateSettings();

  // bind vertex arrays (very important)
  GLuint vao;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  // frame time
  float deltaTime = 0.0f;
  float lastTime = 0.0f;

  // render loop
  while (!glfwWindowShouldClose(window)) {

    // frame time.
    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // handles player inputs
    playerInputs(&player,deltaTime);
    playerMouse(&player);
    //checkPlayer(&player);
    processInput(window);

    //glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);

    // raymarch
    glUseProgram(MarcherID);
    glUniform3f(glGetUniformLocation(MarcherID, "pPos"), player.pos.x,player.pos.y,player.pos.z);
    glUniform3f(glGetUniformLocation(MarcherID, "pDir"), player.dir.x,player.dir.y,player.dir.z);
    
    glDispatchCompute((screenWidth+7)/8,(screenHeight+7)/8,1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // screen
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

void updateSettings() {
    // screen texture (screen color data).
    glGenTextures(1, &screenTex);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, screenWidth, screenHeight);
    glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // sets raymarcher screen sizes
    glUseProgram(MarcherID);
    glUniform1i(glGetUniformLocation(MarcherID, "screenWidth"), screenWidth);
    glUniform1i(glGetUniformLocation(MarcherID, "screenHeight"), screenHeight);

    // sets fragment shader screen sizes
    glUseProgram(ScreenID);
    glUniform1i(glGetUniformLocation(ScreenID, "screenWidth"), screenWidth);
    glUniform1i(glGetUniformLocation(ScreenID, "screenHeight"), screenHeight);

    // set sampler uniform
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glUniform1i(glGetUniformLocation(ScreenID, "screen"), 0);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        //glfwSetCursorPos(window, screenWidth/2.0,screenHeight/2.0);
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
    updateSettings(); // updates settings based on new values.

    // make sure the viewport matches the new window dimensions; note that width and height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height); // resize viewport
}
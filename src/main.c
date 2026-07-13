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
GLuint UpdatesID;
GLuint TerrainID;

// data
const int chunkSize = 64;
GLuint ssbo0; // probe data
size_t ssbo0Size = sizeof(GLuint)*chunkSize*chunkSize*chunkSize/4; // /4 for bitpacking, 8 bit floats
GLuint ssbo1; // material data
size_t ssbo1Size = sizeof(GLuint)*chunkSize*chunkSize*chunkSize/8; // /8 for bitpacking, 4 bit floats, 16 material types, increasable later
GLuint ssbo2; // auxillary/shared data
size_t ssbo2Size = sizeof(GLfloat)*100;

// functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void updateSettings();

int main() {
  //shaderCompile(&TerrainID, GL_COMPUTE_SHADER, "shaders/4.3.terrain.comp");
  //return 0;
  // creates a window
  GLFWwindow *window = createWindow(screenWidth, screenHeight, "I don't know");
  // temp
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  // hide mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // creates player
  player player;
  {vec3 pos = {(float)chunkSize/2.0,(float)chunkSize/2.0,(float)chunkSize/2.0};
  vec3 dir = {0.0,0.0,1.0};
  initializePlayer(&player, pos, dir, 20.0, 0.005, window);}

  // creates SSBOs
  // ssbo0, distance data
  glGenBuffers(1, &ssbo0);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo0);
  glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo0Size, NULL, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo0);

  // ssbo1, material data
  glGenBuffers(1, &ssbo1);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
  glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo1Size, NULL, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo1);

  // ssbo2, auxillary/shared data
  glGenBuffers(1, &ssbo2);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2);
  glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo2Size, NULL, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo2);

  // loads shaders
  // raymarcher
  shaderCompile(&MarcherID, GL_COMPUTE_SHADER, "shaders/4.3.raymarcher.comp");
  MarcherID = linkComputeShader(MarcherID);

  { // screen shader
    GLuint vID;
    GLuint fID ;
    shaderCompile(&vID, GL_VERTEX_SHADER, "shaders/4.3.screenquad.vert");
    shaderCompile(&fID, GL_FRAGMENT_SHADER , "shaders/4.3.screen.frag");
    ScreenID = linkShaders(vID, fID);
  }

  // block update shader
  shaderCompile(&UpdatesID, GL_COMPUTE_SHADER, "shaders/4.3.updates.comp");
  UpdatesID = linkComputeShader(UpdatesID);

  // terrain shader
  shaderCompile(&TerrainID, GL_COMPUTE_SHADER, "shaders/4.3.terrain.comp");
  TerrainID = linkComputeShader(TerrainID);
  
  updateSettings();

  // bind vertex arrays (very important)
  GLuint vao;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  // generate terrain
  glUseProgram(TerrainID);
  glDispatchCompute((chunkSize+3)/4,(chunkSize)/4,(chunkSize+3)/4);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // frame time
  float deltaTime = 0.0f;
  float lastTime = 0.0f;

  // render loop
  while (!glfwWindowShouldClose(window)) {

    // frame time.
    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    //printf("FPS: %f\n",1.0/deltaTime);

    // handles player inputs
    playerInputs(&player,deltaTime);
    playerMouse(&player);

    // clamps player within one chunk.
    {vec3 A = {0.0,0.0,0.0}; vec3 B = {(float)chunkSize-1.0,(float)chunkSize-1.0,(float)chunkSize-1.0};
    clampPlayer(&player, A, B);}

    // process other input
    processInput(window);

    //glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);

    // raymarch
    shaderSetVec3(MarcherID, "pPos", player.pos); // sets player stuff
    shaderSetVec3(MarcherID, "pDir", player.dir);
    shaderSetFloat(MarcherID, "iTime", currentTime);
    
    glDispatchCompute((screenWidth+7)/8,(screenHeight+7)/8,1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // terrain updates
    if (player.mouseClick != 0) {
    vec3 editPos;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2); // reads hit position
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat)*3, &editPos);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(UpdatesID);
    shaderSetVec3(UpdatesID, "uPos", editPos);
    shaderSetFloat(UpdatesID, "uSize", 6.0);
    shaderSetUint(UpdatesID, "uType", 0);
    shaderSetUint(UpdatesID, "uMaterial", 7);
    shaderSetInt(UpdatesID, "uPlace", player.mouseClick);

    glDispatchCompute((chunkSize+3)/4,(chunkSize)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

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
    shaderSetInt(MarcherID, "screenWidth", screenWidth);
    shaderSetInt(MarcherID, "screenHeight", screenHeight);

    // sets fragment shader screen sizes
    shaderSetInt(ScreenID, "screenWidth", screenWidth);
    shaderSetInt(ScreenID, "screenHeight", screenHeight);

    // set sampler uniform
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glUniform1i(glGetUniformLocation(ScreenID, "screen"), 0);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        //glfwSetCursorPos(window, 0,0);
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
    screenWidth = width;
    screenHeight = height;
    updateSettings(); // updates settings based on new values.

    // make sure the viewport matches the new window dimensions; note that width and height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height); // resize viewport
}
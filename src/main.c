// main file, handle uniform setting, object+player handling
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>
#include <Engine/shaders.h>
#include <Engine/chunks.h>

// screen
int screenWidth = 800;
int screenHeight = 600;

// shaders
GLuint ScreenID; // screenshader
GLuint MarcherID; // raymarcher
GLuint UpdatesID; // terrain edits/updates
GLuint TerrainID; // terrain generator

// textures
GLuint screenTex; // screen

// chunk data stuff
const int chunkSize = 63; // chunk size in blocks
const int dataSize = chunkSize+1; // need +1 for boundaries
const int chunkProbes = dataSize*dataSize*dataSize;
const int viewSize = 6; // world size in chunks, 24 max rn
const int viewChunks = viewSize*viewSize*viewSize;
const float full = (float)(dataSize*viewSize);

GLuint ssbo0ID; // probe data
size_t ssbo0Size = sizeof(GLuint)*chunkProbes/4*viewChunks; // /4 for bitpacking, 8 bit floats

GLuint ssbo1ID; // material data
size_t ssbo1Size = sizeof(GLuint)*chunkProbes/8*viewChunks; // /8 for bitpacking, 4 bit floats, 16 material types, increasable later

GLuint ssbo2ID; // chunk mapping data
size_t ssbo2Size = sizeof(GLuint)*viewChunks;
uint* ssbo2Data; // chunk mapping persistently mapped data pointer

// functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void updateSettings();

int main() {

  // creates a window
  GLFWwindow *window = createWindow(screenWidth, screenHeight, "I don't know");
  // temp
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  // hide mouse
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // creates player
  player player;
  {vec3 pos = {38,232,53};//{full/2.0,halfFull/2.0,halfFull/2.0};
  vec3 dir = {0.0,0.0,1.0};
  initializePlayer(&player, pos, dir, 100.0, 0.005, window);}

  // creates SSBOs
  createSSBO(ssbo0ID, ssbo0Size, 0); // distance data
  createSSBO(ssbo1ID, ssbo1Size, 1); // material data
  ssbo2Data = (uint *)createAndPersistentlyMapSSBO(ssbo2ID, ssbo2Size, 2); // chunk index data

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
  
  // updates settings to make sure everything is correct
  updateSettings();

  // bind vertex arrays (very important)
  GLuint vao;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  // generate terrain
  vec3 sp = {0.0,0.0,0.0};
  genSpawnChunks(sp);

  // frame time
  float deltaTime = 0.0f;
  float lastTime = 0.0f;
  vec3 p = {0.0,0.0,0.0};

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
    {vec3 A = {0.0,0.0,0.0}; vec3 B = {full-viewSize,full-viewSize,full-viewSize};
    clampPlayer(&player, A, B);}
    //checkPlayer(&player);

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
        vec3 target = add_f3(player.pos,multiply_f3xf(player.dir,12.0));
        applyUpdate(target, player.mouseClick, 0, 6.0, 7);
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
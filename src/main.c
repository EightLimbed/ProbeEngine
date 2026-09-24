// main file, handle uniform setting, object+player handling
#include "Engine/types.h"
#include <Engine/load.h>
#include <Engine/player.h>
#include <GLFW/glfw3.h>
#include <Engine/shaders.h>
#include <Engine/chunks.h>
#include <Engine/physics.h>

// screen
int screenWidth = 800;
int screenHeight = 600;

// shaders
GLuint ScreenID; // screenshader
GLuint MarcherID; // raymarcher
GLuint LightingID; // lighting pass 
GLuint SunID; // sun lighting pass 
GLuint UpdatesID; // terrain edits/updates
GLuint TerrainID; // terrain generator
GLuint OccupancyID; // second stage to terrain, sets occupancy and stuff
GLuint ColliderID; // stage that gets collision surface around player.
GLuint ResetID; // index occupancy resetter

// screen textures
GLuint colorTex; // screen colors
GLuint depthTex; // depth and surface normal data
GLuint lightTex; // lighting data

// lighting
const uint lightSamples = 6; // samples for lighting
const uint lightFrames = 6;
const uint lightFidelity = 2;
int screenHeightLight;
int screenWidthLight;

// chunk data stuff
const uint chunkCut = 10; // amount to divide max memory by
const uint chunkSize = 32; // chunk size in blocks
const uint chunkProbes = chunkSize*chunkSize*chunkSize;
const uint viewSize = 32; // world size in chunks
const uint viewChunks = viewSize*viewSize*viewSize;

// collisions stuff
const uint simSize = 8*32; // simulation distance in probes
const uint simProbes = simSize*simSize*simSize;
const uint simFidelity = 4; // amount to cut collision buffer detail by in each axis

// world stuff
const uint allotedChunks = viewChunks/chunkCut;
const float axisSize = (float)(chunkSize*viewSize);
const float center = (float)(viewSize/2.0)*(float)chunkSize;
vec3 worldPos; // position of world, for local positioning

// ssbos
GLuint ssbo0ID; // probe data
size_t ssbo0Size = (sizeof(GLuint)*allotedChunks*chunkProbes+3)/4; // /4 for bitpacking, 8 bit floats
uint* surfaceData; // chunk mapping persistently mapped data pointer

GLuint ssbo1ID; // material data
size_t ssbo1Size = (sizeof(GLuint)*allotedChunks*chunkProbes+7)/8; // /8 for bitpacking, 4 bit floats, 16 material types, increasable later

GLuint ssbo2ID; // chunk bitmask data,
size_t ssbo2Size = (sizeof(GLuint)*(viewChunks)*2); // 0-viewChunks holds slots, anything past holds flags
uint* chunkData;

GLuint ssbo3ID; // low res collision sdf
const uint cubedSim = simFidelity*simFidelity*simFidelity;
size_t ssbo3Size = (sizeof(GLuint)*(simProbes+cubedSim)/(cubedSim)); // lower resolution probe grid for collisions
uint* colliderData;

// functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void updateScreenSettings();

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
  {vec3 pos = {0.0,-100.0,0.0};
  vec3 dir = {0.0,0.0,1.0};
  initializePlayer(&player, pos, dir, 24.0, 100.0, 0.005, window);}
  worldPos = getChunkPos(player.pos); // update world position

  // creates SSBOs
  //surfaceData = (uint *)createAndPersistentlyMapSSBO(ssbo0ID, ssbo0Size, 0); // distance data
  createSSBO(ssbo0ID, ssbo0Size, 0); // distance data
  createSSBO(ssbo1ID, ssbo1Size, 1); // material data
  chunkData = (uint *)createAndPersistentlyMapSSBO(ssbo2ID, ssbo2Size, 2); // index data
  colliderData = (uint *)createAndPersistentlyMapSSBO(ssbo3ID, ssbo3Size, 3); // collision sdf data

  // loads shaders
  // raymarcher
  shaderCompile(&MarcherID, GL_COMPUTE_SHADER, "shaders/4.3.raymarcher.comp");
  MarcherID = linkComputeShader(MarcherID);

  // lighting shader
  shaderCompile(&LightingID, GL_COMPUTE_SHADER, "shaders/4.3.lighting.comp");
  LightingID = linkComputeShader(LightingID);

  // sun light shader shader
  shaderCompile(&SunID, GL_COMPUTE_SHADER, "shaders/4.3.sun.comp");
  SunID = linkComputeShader(SunID);

  { // screen shader
    GLuint vID;
    GLuint fID ;
    shaderCompile(&vID, GL_VERTEX_SHADER, "shaders/4.3.screenquad.vert");
    shaderCompile(&fID, GL_FRAGMENT_SHADER , "shaders/4.3.screen.frag");
    ScreenID = linkShaders(vID, fID);
  }

  // update shader
  shaderCompile(&UpdatesID, GL_COMPUTE_SHADER, "shaders/4.3.updates.comp");
  UpdatesID = linkComputeShader(UpdatesID);

  // terrain gen shader
  shaderCompile(&TerrainID, GL_COMPUTE_SHADER, "shaders/4.3.terrain.comp");
  TerrainID = linkComputeShader(TerrainID);

  // occupancy (terrains stage 2) shader
  shaderCompile(&OccupancyID, GL_COMPUTE_SHADER, "shaders/4.3.occupancy.comp");
  OccupancyID = linkComputeShader(OccupancyID);

  // chunk reset shader
  shaderCompile(&ResetID, GL_COMPUTE_SHADER, "shaders/4.3.reset.comp");
  ResetID = linkComputeShader(ResetID);

  // update shader
  shaderCompile(&ColliderID, GL_COMPUTE_SHADER, "shaders/4.3.collider.comp");
  ColliderID = linkComputeShader(ColliderID);
  
  // updates settings to make sure everything is correct
  updateScreenSettings();

  // bind vertex arrays (very important)
  GLuint vao;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  // generate terrain
  resetChunks();
  genSpawnChunks();
  updatecolliderData();

  // frame time
  float deltaTime = 0.0f;
  float lastTime = 0.0f;
  float maxDelta = 0.0f;
  float minDelta = 1e20f;

  // lighting frame
  int lightFrame = 0;

  // old world pos for shifting
  vec3 owp = worldPos;

  // render loop
  while (!glfwWindowShouldClose(window)) {

    // frame time.
    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // fps display
    printf("FPS: %.2f \nMin FPS: %.2f \nMax FPS: %.2f\n\033[3A\r",1.0/deltaTime, 1.0/maxDelta, 1.0/minDelta);
    if (deltaTime>maxDelta) maxDelta = deltaTime;
    if (deltaTime<minDelta) minDelta = deltaTime;

    // handles player inputs
    playerInputs(&player,deltaTime);
    playerMouse(&player);
    playerPhysics(&player);
    worldPos = getChunkPos(player.pos); // update world position

    // terrain updates
    if (player.mouseClick != 0) {
        // apply update
        vec3 target = raycast(player.pos, player.dir, 128.0);
        applyUpdate(target, player.mousePress, 0, 6.0, 7);
        updatecolliderData();
    }

    //printf("World Position: (%2f, %2f, %2f)\n", worldPos.x, worldPos.y, worldPos.z);

    // terrain gen
    if (!equal_f3(worldPos, owp)) {
        ivec3 shift = {(int)(worldPos.x-owp.x)/(int)chunkSize,
                       (int)(worldPos.y-owp.y)/(int)chunkSize,
                       (int)(worldPos.z-owp.z)/(int)chunkSize};

        shiftChunks(shift);
        updatecolliderData();
    }
        
    owp = worldPos;

    // apply terrain gen
    followChunkQueue();

    // process other input
    processInput(window);

    // raymarch
    glUseProgram(MarcherID);
    shaderSetVec3(MarcherID, "pPos", player.pos); // sets player stuff
    shaderSetVec3(MarcherID, "worldPos", worldPos); // sets player stuff
    shaderSetVec3(MarcherID, "pDir", player.dir);
    shaderSetFloat(MarcherID, "iTime", currentTime);
    
    glDispatchCompute((screenWidth+7)/8,(screenHeight+7)/8,1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // lighting
    lightFrame = (lightFrame+1)%lightFrames;
    glUseProgram(LightingID);
    shaderSetVec3(LightingID, "worldPos", worldPos); // sets player stuff
    shaderSetVec3(LightingID, "pPos", player.pos); // sets player stuff
    shaderSetVec3(LightingID, "pDir", player.dir);
    shaderSetFloat(LightingID, "iTime", currentTime);
    shaderSetInt(LightingID, "lightFrame", lightFrame);
    
    glDispatchCompute((screenWidthLight+3)/4,(screenHeightLight+3)/4,(lightSamples+3)/4);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // sun lighting
    glUseProgram(SunID);
    shaderSetVec3(SunID, "worldPos", worldPos); // sets player stuff
    shaderSetVec3(SunID, "pPos", player.pos); // sets player stuff
    shaderSetVec3(SunID, "pDir", player.dir);
    shaderSetFloat(SunID, "iTime", currentTime);
    
    glDispatchCompute((screenWidthLight+7)/8,(screenHeightLight+7)/8,1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // screen
    glUseProgram(ScreenID);

    if (glfwGetKey(window,GLFW_KEY_R)==GLFW_PRESS) {
        // reset fps display stuff if necessary
        maxDelta = 0.0f;
        minDelta = 1e20f;
    }
    // draw triangles
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // poll events and draw screen
    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  // end
  printf("\n\n\n");
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

void updateScreenSettings() {
    screenWidthLight = (screenWidth+lightFidelity-1)/lightFidelity;
    screenHeightLight = (screenHeight+lightFidelity-1)/lightFidelity;

    // sets fragment shader screen sizes
    glUseProgram(ScreenID);
    shaderSetInt(ScreenID, "screenWidth", screenWidth);
    shaderSetInt(ScreenID, "screenHeight", screenHeight);
    shaderSetInt(ScreenID, "lightSamples", lightSamples);
    shaderSetInt(ScreenID, "lightFrames", lightFrames);

    // sets raymarcher screen sizes
    glUseProgram(MarcherID);
    shaderSetInt(MarcherID, "screenWidth", screenWidth);
    shaderSetInt(MarcherID, "screenHeight", screenHeight);

    // sets lighting shader screen sizes
    glUseProgram(LightingID);
    shaderSetInt(LightingID, "screenWidth", screenWidthLight);
    shaderSetInt(LightingID, "screenHeight", screenHeightLight);
    shaderSetInt(LightingID, "lightSamples", lightSamples);

    // sets sunlight shader screen stuff
    glUseProgram(SunID);
    shaderSetInt(SunID, "screenWidth", screenWidthLight);
    shaderSetInt(SunID, "screenHeight", screenHeightLight);
    shaderSetInt(SunID, "lightFrames", lightFrames);
    shaderSetInt(SunID, "lightSamples", lightSamples);

    // screen texture (screen color data).
    glDeleteTextures(1, &colorTex);
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, screenWidth, screenHeight);
    glBindImageTexture(0, colorTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // depth texture (depth and normal data).
    glDeleteTextures(1, &depthTex);
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, screenWidth, screenHeight);
    glBindImageTexture(1, depthTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // lighting texture (colored lighting). 3D texture, each layer is a pass
    glDeleteTextures(1, &lightTex);
    glGenTextures(1, &lightTex);
    glBindTexture(GL_TEXTURE_3D, lightTex);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, screenWidthLight, screenHeightLight, lightSamples*lightFrames);
    glBindImageTexture(2, lightTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // set lighting image stuff
    glUseProgram(depthTex);
    
    glActiveTexture(GL_TEXTURE1); // set depth sampler uniform
    glBindTexture(GL_TEXTURE_2D, depthTex);
    shaderSetInt(ScreenID, "depthMap", 1);

    // set screen image stuff
    glUseProgram(ScreenID);

    glActiveTexture(GL_TEXTURE0); // set color sampler uniform
    glBindTexture(GL_TEXTURE_2D, colorTex);
    shaderSetInt(ScreenID, "colorMap", 0);

    glActiveTexture(GL_TEXTURE1); // set depth sampler uniform
    glBindTexture(GL_TEXTURE_2D, depthTex);
    shaderSetInt(ScreenID, "depthMap", 1);

    glActiveTexture(GL_TEXTURE2); // set light sampler uniform
    glBindTexture(GL_TEXTURE_3D, lightTex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    shaderSetInt(ScreenID, "lightMap", 2);
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

    updateScreenSettings(); // updates settings based on new values.

    // make sure the viewport matches the new window dimensions; note that width and height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height); // resize viewport
}
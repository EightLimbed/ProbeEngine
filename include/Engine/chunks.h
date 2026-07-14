#pragma once
#include <Engine/types.h>
#include <Engine/shaders.h>

// data stuff
extern const int chunkSize;
extern const int worldSize; // world size in chunks
extern GLuint ssbo0ID; // probe data
extern size_t ssbo0Size; // /4 for bitpacking, 8 bit floats
extern GLuint ssbo1ID; // material data
extern size_t ssbo1Size; // /8 for bitpacking, 4 bit floats, 16 material types, increasable later
extern GLuint ssbo2ID; // chunk mapping data
extern size_t ssbo2Size;

// terrain generator
extern GLuint TerrainID;

// need function to generate chunk at a position

// need function for the first pass upon loading where you assign a index to every chunk position (will do grid with posToIndex function)
void genSpawnChunks(vec3 pos) {
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "cPos", pos);
    glDispatchCompute((chunkSize+3)/4,(chunkSize)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

// need function to update chunk indexes when player moves. Will set indexes moved out of to new chunks indexes.



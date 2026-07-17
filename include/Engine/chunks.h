// handles chunk indexing and generation, uses values set in main.c
#pragma once
#include <Engine/types.h>
#include <Engine/shaders.h>

// data stuff
extern const int dataSize;
extern const int viewSize; // view size in chunks
extern const int viewChunks;
extern GLuint ssbo2ID; // chunk mapping data
extern size_t ssbo2Size;

// pointer to chunk buffer data
extern uint* ssbo2Data;

// terrain generator
extern GLuint TerrainID;

// updater
extern GLuint UpdatesID;

// takes position of chunk and spits out index.
unsigned int posToChunkIndex(vec3 p) {
    return (uint)p.x+viewChunks*((uint)p.y+viewChunks*(uint)p.z);
}

// need function to generate chunk at a position
void generateChunk(vec3 pos) {
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "cPos", pos);
    glDispatchCompute((dataSize+3)/4,(dataSize+3)/4,(dataSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

// need function for the first pass upon loading where you assign a index to every chunk position (will do grid with posToIndex function)
void genSpawnChunks(vec3 pos) {
    // pos currently unused
    for (int x = 0; x < viewSize; x++) 
    for (int y = 0; y < viewSize; y++)
    for (int z = 0; z < viewSize; z++) {
        vec3 p = {(float)x,(float)y,(float)z};
        generateChunk(p); // generates chunk at position
    }
}

// need function to update chunk indexes when player moves. Will set indexes moved out of to new chunks indexes.

// need function to apply updates to chunks when necessary
void applyUpdate(vec3 target, int click, uint type, float size, uint material) {
    for (int x = -1; x < 2; x++) 
    for (int y = -1; y < 2; y++)
    for (int z = -1; z < 2; z++) {
        vec3 offset = {(float)x,(float)y,(float)z};
        glUseProgram(UpdatesID);
        
        shaderSetVec3(UpdatesID, "uPos", target);
        shaderSetInt(UpdatesID, "uClick", click);
        shaderSetUint(UpdatesID, "uType", type);
        shaderSetFloat(UpdatesID, "uSize", size);
        shaderSetUint(UpdatesID, "uMaterial", material);
        
        shaderSetVec3(UpdatesID, "cOffset", offset); // chunk offset position
        

        glDispatchCompute((dataSize+3)/4,(dataSize+3)/4,(dataSize+3)/4);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}

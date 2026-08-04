// handles chunk indexing and generation, uses values set in main.c
#pragma once
#include "Engine/shaders.h"
#include "shaders.h"
#include "types.h"
#include <Engine/types.h>
#include <Engine/shaders.h>

// world specs
extern const int chunkSize;
extern const int viewSize; // view size in chunks
extern const int viewChunks;
extern const int chunkProbes;
extern const float center;


// buffer data
extern GLuint ssbo2ID; // chunk mapping data
extern size_t ssbo2Size;
extern vec3 worldPos; // position of world, for local positioning

// pointer to surface data
extern uint* surfaceData;

// pointer to index data
extern uint* indexData;

// terrain generator
extern GLuint TerrainID;

// updater
extern GLuint UpdatesID;

// empty checker
extern GLuint CheckerID;

// empty chunk ID
const uint empty = 0xFFFFFFFFu;

vec3 getChunkPos(vec3 p) {
    float cs = (float)chunkSize;
    vec3 cp = multiply_f3xf(floor_f3(multiply_f3xf(p, 1.0f/cs)),cs);
    return cp;
}

vec3 getChunkPosCentered(vec3 p) {
    float cs = (float)chunkSize;
    vec3 cp = multiply_f3xf(floor_f3(multiply_f3xf(p, 1.0f/cs)),cs);
    vec3 ct = {center,center,center};
    //vec3 print = multiply_f3xf(floor_f3(multiply_f3xf(p, 1.0f/cs)),cs);
    //printf("Chunk Pos: (%f,%f,%f)\n",print.x,print.y,print.z);
    return subtract_f3(cp, ct);
}

uvec3 getLocalPos(vec3 p) {
    uvec3 lp = mod_u3xu(to_uvec3((add_f3(floor_f3(p),worldPos))), chunkSize*viewSize);
    return lp;
}

// gets index in index data
uint posToChunkIndex(uvec3 lp) {
    uvec3 cp = divide_u3xu(lp, chunkSize);
    return cp.x+viewSize*(cp.y+viewSize*cp.z);
}

// generates chunk at position, automatically puts it at nearest empty chunk index.
void generateChunk(vec3 pos) {
    // get current chunk
    uvec3 lp = getLocalPos(pos);
    uint uci = posToChunkIndex(lp); 
    indexData[uci] = empty; // assume chunk is empty for now

    // iterate until nearest empty chunk, and go there.
    uint slot = empty;
    for (uint i = 0u; i < viewChunks; i++) {
        if (indexData[i] == empty) {
            slot = i*chunkProbes;  // make fit in data indexes.
            break;
        }
    }

    // generates chunk, which also checks if chunk is full and sets its slot
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "cPos", subtract_f3(pos, worldPos));
    shaderSetUint(TerrainID, "slot", slot); // set chunk index 
    shaderSetUint(TerrainID, "cIndex", uci);
    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();
}

void resetChunks() {
    glUseProgram(CheckerID);
    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();
}

void updateChunk(vec3 target, vec3 cPos, int click, uint type, float size, uint material) {
    glUseProgram(UpdatesID);
    
    shaderSetVec3(TerrainID, "worldPos", worldPos); // update world position
    shaderSetVec3(UpdatesID, "uPos", target);
    shaderSetInt(UpdatesID, "uClick", click);
    shaderSetUint(UpdatesID, "uType", type);
    shaderSetFloat(UpdatesID, "uSize", size);
    shaderSetUint(UpdatesID, "uMaterial", material);
    shaderSetVec3(UpdatesID, "cPos", cPos); // chunk offset position

    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

// need function for the first pass upon loading where you assign a index to every chunk position (will do grid with posToIndex function)
void genSpawnChunks() {
    for (uint i = 0u; i < viewChunks; i--) {
        if (indexData[i] != empty) {
            printf("Incorrect Initialization at: %u\n", i);
            return;
        }
    }
    for (int x = 0; x < viewSize; x++) 
    for (int y = 0; y < viewSize; y++)
    for (int z = 0; z < viewSize; z++) {
        vec3 p = {(float)x,(float)y,(float)z};
        vec3 cPos = add_f3(multiply_f3xf(p, (float)chunkSize), worldPos);
        generateChunk(cPos); // generates chunk at position
        //checkChunk(cPos);
    }
    // checks
    for (uint i = 0; i < viewChunks; i++) {
        uint reduced = indexData[i]/chunkProbes;
        if (indexData[i] == empty)  printf("Chunk %u slot is empty\n", i);
        else printf("Chunk %u slot is: %u, with difference of: %u\n", i, reduced, i-reduced);
    }
}

// need function to update chunk indexes when player moves. Will set indexes moved out of to new chunks indexes.
// positioning wrong, going in right place in memory tho
void shiftChunks(ivec3 shift) {
    // shift x
    if (abs(shift.x) > 0) {
        for (int y = 0; y < viewSize; y++)
        for (int z = 0; z < viewSize; z++) {
            ivec3 ip = {(shift.x>0) ? shift.x*viewSize-1 : 0, y, z}; // need -1, likely because shifting or something idk
            vec3 cPos = multiply_f3xf(to_vec3(ip), (float)chunkSize);

            generateChunk(cPos);
        }
    }
    // shift z
    if (abs(shift.z) > 0) {
        for (int x = 0; x < viewSize; x++)
        for (int y = 0; y < viewSize; y++) {
            ivec3 ip = {x, y, (shift.z>0) ? shift.z*viewSize-1 : 0};
            vec3 cPos = multiply_f3xf(to_vec3(ip), (float)chunkSize);
            vec3 uPos = subtract_f3(cPos, multiply_f3xf(to_vec3(shift), (float)chunkSize));

            generateChunk(cPos);
        }
    }
    // shift y
    if (abs(shift.y) > 0) {
        for (int x = 0; x < viewSize; x++)
        for (int z = 0; z < viewSize; z++) {
            ivec3 ip = {x, (shift.y>0) ? shift.y*viewSize-1 : 0, z};
            vec3 cPos = multiply_f3xf(to_vec3(ip), (float)chunkSize);
            vec3 uPos = subtract_f3(cPos, multiply_f3xf(to_vec3(shift), (float)chunkSize));

            generateChunk(cPos);
        }
    }
    //checkSpawnChunks();


    // apply edits saved
}

// applies updates when necessary
void applyUpdate(vec3 target, int click, uint type, float size, uint material) {
    for (int x = -1; x < 2; x++) 
    for (int y = -1; y < 2; y++)
    for (int z = -1; z < 2; z++) {
        vec3 offset = {(float)x,(float)y,(float)z};
        vec3 cPos = add_f3(getChunkPos(target),multiply_f3xf(offset, (float)chunkSize));
        updateChunk(target, cPos, click, type, size, material);
    }
}

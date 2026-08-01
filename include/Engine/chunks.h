// handles chunk indexing and generation, uses values set in main.c
#pragma once
#include <Engine/types.h>
#include <Engine/shaders.h>

// world specs
extern const int chunkSize;
extern const int viewSize; // view size in chunks
extern const int viewChunks;
extern const float center;

// buffer data
extern GLuint ssbo2ID; // chunk mapping data
extern size_t ssbo2Size;
extern vec3 worldPos; // position of world, for local positioning

// pointer to surface data
extern uint* surfaceData;

// terrain generator
extern GLuint TerrainID;

// updater
extern GLuint UpdatesID;

// empty checker
extern GLuint CheckerID;

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

// generates chunk at position
void generateChunk(vec3 pos) {
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "worldPos", worldPos); // update world position
    shaderSetVec3(TerrainID, "cPos", pos);
    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void checkChunk(vec3 pos) {
    glUseProgram(CheckerID);
    shaderSetVec3(TerrainID, "worldPos", worldPos); // update world position
    shaderSetVec3(CheckerID, "cPos", pos);
    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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
    for (int x = 0; x < viewSize; x++) 
    for (int y = 0; y < viewSize; y++)
    for (int z = 0; z < viewSize; z++) {
        vec3 p = {(float)x,(float)y,(float)z};
        vec3 cPos = add_f3(multiply_f3xf(p, (float)chunkSize), worldPos);
        generateChunk(cPos); // generates chunk at position
        checkChunk(cPos);
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
            checkChunk(add_f3(cPos, worldPos)); // add worldPos for wrapping
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
            checkChunk(add_f3(cPos, worldPos)); // add worldPos for wrapping
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
            checkChunk(add_f3(cPos, worldPos)); // add worldPos for wrapping
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
        checkChunk(getChunkPosCentered(cPos)); // chunks are being updated around center, center checks
    }
}

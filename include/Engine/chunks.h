// handles chunk indexing and generation, uses values set in main.c
#pragma once
#include "Engine/shaders.h"
#include "shaders.h"
#include "types.h"
#include <Engine/types.h>
#include <Engine/shaders.h>

// world specs
extern const uint chunkSize;
extern const uint viewSize; // view size in chunks
extern const uint viewChunks;
extern const uint chunkProbes;
extern const float center;

// buffer data
extern GLuint ssbo2ID; // chunk mapping data
extern size_t ssbo2Size;
extern vec3 worldPos; // position of world, for local positioning

// pointer to surface data
extern uint* surfaceData;

// pointer to index data
extern uint* indexData;
uint* slotOccupancy; // occupancies of slots

// terrain generator
extern GLuint TerrainID;

// updater
extern GLuint UpdatesID;

// index occupancy resetter
extern GLuint ResetID;

// empty chunk ID
const uint empty = 0xFFFFFFFFu;

vec3 getChunkPos(vec3 p) {
    float cs = (float)chunkSize;
    vec3 cp = multiply_f3xf(floor_f3(multiply_f3xf(p, 1.0f/cs)),cs);
    return cp;
}

uvec3 getLocalPos(vec3 p) {
    uvec3 lp = mod_u3xu(vec3_to_uvec3((add_f3(floor_f3(p),worldPos))), chunkSize*viewSize);
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
    if (indexData[uci] != empty) {
        // essentially gets the slot that that chunk index was assigned
        slotOccupancy[indexData[uci]] = 0u; // empty old slot this chunk was filling
        indexData[uci] = empty; // assume chunk is empty for now
    }

    //iterate until nearest empty chunk, and go there.
    uint slot;
    for (uint i = 0u; i < viewChunks; i++) {
        if (slotOccupancy[i] == 0u) { // if empty chunk found, set slot.
            slot = i;
            slotOccupancy[i] = 1u; // flag chunk as full
            break;
        }
    }

    // generates chunk, which also checks if chunk is full and sets its slot
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "cPos", pos);
    shaderSetUint(TerrainID, "slot", slot); // set slot
    shaderSetUint(TerrainID, "cIndex", uci); // set chunk index
    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();

    if (indexData[uci] == empty) slotOccupancy[slot] = 0u; // if chunk not filled keep found slot empty
}

// gets data structures ready for chunks
void resetChunks() {
    // create slotOccupancy map
    free(slotOccupancy);
    slotOccupancy = calloc(viewChunks,sizeof(uint));
    // set all indexes to empty
    glUseProgram(ResetID);
    glDispatchCompute((viewSize+3)/4,(viewSize+3)/4,(viewSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();
}

void printChunkData() {
    uint r = 0u;
    for (uint i = 0; i < viewChunks; i++) {
        uint reduced = indexData[i]/chunkProbes;
        if (indexData[viewChunks-i-1] == empty) r++;
        //if (indexData[i] == empty)  printf("Chunk %u slot is empty\n", i);
        //else printf("Chunk %u slot is: %u, with difference of: %u\n", i, reduced, i-reduced);
    }
    
    printf("%u empties out of %u chunks (1 in %u full).\n",r,viewChunks,viewChunks/(viewChunks-r));
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
    // early out if incorrect initialization
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
    }

    // checks
    printChunkData();
}

// need function to update chunk indexes when player moves. Will set indexes moved out of to new chunks indexes.
// positioning wrong, going in right place in memory tho
void shiftChunks(ivec3 shift) {
    // shift x
    if (shift.x != 0) {
        for (int y = 0; y < viewSize; y++)
        for (int z = 0; z < viewSize; z++) {
            ivec3 ip = {(shift.x>0) ? shift.x*viewSize-1 : 0, y-viewSize/2, z-viewSize/2}; // need -1, likely because shifting or something idk
            vec3 cPos = multiply_f3xf(ivec3_to_vec3(ip), (float)chunkSize);

            generateChunk(cPos);
        }
    }
    // shift z
    if (shift.z != 0) {
        for (int x = 0; x < viewSize; x++)
        for (int y = 0; y < viewSize; y++) {
            ivec3 ip = {x-viewSize/2, y-viewSize/2, (shift.z>0) ? shift.z*viewSize-1 : 0};
            vec3 cPos = multiply_f3xf(ivec3_to_vec3(ip), (float)chunkSize);

            generateChunk(cPos);
        }
    }
    // shift y
    if (shift.y != 0) {
        for (int x = 0; x < viewSize; x++)
        for (int z = 0; z < viewSize; z++) {
            ivec3 ip = {x-viewSize/2, (shift.y>0) ? shift.y*viewSize-1 : 0, z-viewSize/2};
            vec3 cPos = multiply_f3xf(ivec3_to_vec3(ip), (float)chunkSize);

            generateChunk(cPos);
        }
    }
    printChunkData();


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

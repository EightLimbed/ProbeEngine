// handles chunk indexing and generation, uses values set in main.c
#pragma once
#include "glad/glad.h"
#include <Engine/types.h>
#include <Engine/shaders.h>

// memory data
extern const uint cut;

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
const uint empty = 0xFFFFFFFFu; // flag for if chunk isn't loaded

vec3 getChunkPos(vec3 p) {
    float cs = (float)chunkSize;
    vec3 cp = multiply_f3xf(floor_f3(multiply_f3xf(p, 1.0f/cs)),cs);
    return cp;
}

uvec3 getLocalPos(vec3 p) {
    uvec3 lp = vec3_to_uvec3(mod_f3xf(add_f3(floor_f3(p),worldPos), (float)chunkSize*viewSize));
    return lp;
}

// gets index in index data
uint posToChunkIndex(uvec3 lp) {
    uvec3 cp = divide_u3xu(lp, chunkSize);
    return cp.x+viewSize*(cp.y+viewSize*cp.z);
}

// gets data structures ready for chunks
void resetChunks() {
    // create slotOccupancy map
    free(slotOccupancy);
    slotOccupancy = calloc(viewChunks/cut,sizeof(uint));
    // set all indexes to empty
    glUseProgram(ResetID);
    glDispatchCompute((viewSize+3)/4,(viewSize+3)/4,(viewSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();
}

void dispatchChunk(vec3 cPos, uint slot, uint cIndex) {
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "cPos", cPos);
    shaderSetUint(TerrainID, "slot", slot); // set slot
    shaderSetUint(TerrainID, "cIndex", cIndex); // set chunk index

    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

// generates chunk at global chunk position, automatically puts it at nearest empty chunk index.
void generateChunk(vec3 cPos) {
    // get current chunk
    uvec3 lp = getLocalPos(subtract_f3(cPos, worldPos));
    uint cIndex = posToChunkIndex(lp);

    // make sure old chunk gets overwritten
    if (indexData[cIndex] != empty) {
        // essentially gets the slot that that chunk index was assigned
        slotOccupancy[indexData[cIndex]] = 0u; // empty old slot this chunk was filling
        indexData[cIndex] = empty; // assume chunk is empty for now
    }

    // find an empty chunk, and go there
    uint slot = empty; // if slot stays empty, don't generate
    uint hash = hash_uint(cIndex); // hash for easier search
    for (uint i = 0u; i < viewChunks/cut; i++) {
        uint index = (i+hash)%(viewChunks/cut); // modulate within alloted space
        if (slotOccupancy[index] == 0u) { // if empty chunk found, set slot.
            slot = index;
            slotOccupancy[index] = 1u; // flag chunk as full
            //printf("%u steps required for chunk.\n", i);
            break;
        }
    } if (slot == empty) return;  // if no space found, do nothing
    indexData[viewChunks] = 0; // reset flag

    // generates chunk, which also checks if chunk is full and sets its slot
    
    dispatchChunk(cPos, slot, cIndex);
    createFenceGPU();

    // last index used as flag for fully empty or full.
    if (indexData[viewChunks] == 3) indexData[cIndex] = slot;
 
    else slotOccupancy[slot] = 0u; // if chunk not filled keep found slot empty
}

// updates chunk at position
void updateChunk(vec3 target, vec3 cPos, int click, uint type, float size, uint material) {
    // get current chunk
    uvec3 lp = getLocalPos(subtract_f3(cPos,worldPos));
    //printf("Update lp: (%u,%u,%u), ",lp.x,lp.y,lp.z);
    uint cIndex = posToChunkIndex(lp);

    uint slot = empty;
    if (indexData[cIndex] != empty) {
        slot = indexData[cIndex]; // get old chunk slot to edit if exists
        //printf("Slot for chunk already exists, ");
    }

    else for (uint i = 0u; i < viewChunks/cut; i++) { // otherwise find empty slot for one
        if (slotOccupancy[i] == 0u) { // if empty chunk found, set slot.
            slot = i;
            slotOccupancy[i] = 1u; // flag chunk as full
            //dispatchChunk(cPos, slot, cIndex); // reset chunk
            //printf("Needed to find slot for chunk, ");
            break;
        }
    } if (slot == empty) return; // if no space found, do nothing
    //printf("Using slot %u at %u chunk index.\n",slot, cIndex);

    // generates chunk, which also checks if chunk is full and sets its slot
    glUseProgram(UpdatesID);

    shaderSetUint(UpdatesID, "slot", slot); // set slot
    shaderSetUint(UpdatesID, "cIndex", cIndex); // set chunk index, no need to set if assuming full from edits

    shaderSetVec3(UpdatesID, "uPos", subtract_f3(cPos, target)); // pass in local update position
    shaderSetInt(UpdatesID, "uClick", click);
    shaderSetUint(UpdatesID, "uType", type);
    shaderSetFloat(UpdatesID, "uSize", size);
    shaderSetUint(UpdatesID, "uMaterial", material);

    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();

    indexData[cIndex] = slot; // assume chunk gets filled, do this after compute shader to ensure clearing functions

    // if (indexData[viewChunks] == 3) indexData[cIndex] = slot;
 
    // else slotOccupancy[slot] = 0u; // if chunk not filled keep found slot empty
}

void printChunkData() {
    uint r = 0u;
    uint p = 0u;
    for (uint i = 0; i < viewChunks; i++) {
        uint reduced = indexData[i]/chunkProbes;
        if (indexData[i] == empty) r++;
        if (slotOccupancy[i] == 0u) p++;
        //if (indexData[i] == empty)  printf("Chunk %u slot is empty\n", i);
        //else printf("Chunk %u slot is: %u, with difference of: %u\n", i, reduced, i-reduced);
    }

    if (viewChunks-r > 0u) printf("%u empties taking %u slots out of %u chunks (1 in %u full).\n",r,p,viewChunks,viewChunks/(viewChunks-r));
    else printf("No chunks full.\n");
}

// need function for the first pass upon loading where you assign a index to every chunk position (will do grid with posToIndex function)
void genSpawnChunks() {
    //float time = glfwGetTime();
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
        vec3 cPos = add_f3(multiply_f3xf(p, (float)chunkSize), worldPos); // adding worldPos because generateChunk works on worldPos
        generateChunk(cPos); // generates chunk at global position
    }
    //printf("Took %f seconds to generate spawn chunks.\n", glfwGetTime()-time);

    // checks
    printChunkData();
}

// 'shifts' chunks when player moves, localPos actually handles shifting, this just regenerates new chunks.
void shiftChunks(ivec3 shift) {
    // shift x
    if (shift.x != 0) {
        for (int y = 0; y < viewSize; y++)
        for (int z = 0; z < viewSize; z++) {
            ivec3 ip = {(shift.x>0) ? shift.x*viewSize-1 : 0, y, z}; // need -1, likely because shifting or something idk
            vec3 cPos = add_f3(multiply_f3xf(ivec3_to_vec3(ip), (float)chunkSize),worldPos);

            generateChunk(cPos);
        }
    }
    // shift z
    if (shift.z != 0) {
        for (int x = 0; x < viewSize; x++)
        for (int y = 0; y < viewSize; y++) {
            ivec3 ip = {x, y, (shift.z>0) ? shift.z*viewSize-1 : 0};
            vec3 cPos = add_f3(multiply_f3xf(ivec3_to_vec3(ip), (float)chunkSize),worldPos);

            generateChunk(cPos);
        }
    }
    // shift y
    if (shift.y != 0) {
        for (int x = 0; x < viewSize; x++)
        for (int z = 0; z < viewSize; z++) {
            ivec3 ip = {x, (shift.y>0) ? shift.y*viewSize-1 : 0, z};
            vec3 cPos = add_f3(multiply_f3xf(ivec3_to_vec3(ip), (float)chunkSize),worldPos);

            generateChunk(cPos);
        }
    }
    //printChunkData();


    // apply edits saved
}

// applies updates when necessary
// takes global position of update, and properties of it
void applyUpdate(vec3 target, int click, uint type, float size, uint material) {
    uint count = 0u;
    for (int x = -1; x < 2; x++) 
    for (int y = -1; y < 2; y++)
    for (int z = -1; z < 2; z++) {
        vec3 offset = {(float)x,(float)y,(float)z};
        vec3 ctarget = subtract_f3(target, (vec3){center,center,center}); // centered target
        vec3 cPos = add_f3(getChunkPos(ctarget),multiply_f3xf(offset, (float)chunkSize));
        //printf("Chunk %u ", count);
        updateChunk(ctarget, cPos, click, type, size, material); // updates chunk at global position
        //printf("Offset: (%f,%f,%f)",cPos.x,cPos.y,cPos.z);
        count ++;
    }
    printChunkData();
    printf("\n");
}

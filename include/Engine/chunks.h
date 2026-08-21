// handles chunk indexing and generation, uses values set in main.c
#pragma once
#include "glad/glad.h"
#include <Engine/types.h>
#include <Engine/shaders.h>
#include <time.h>

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
extern uint* chunkData;

// shaders
extern GLuint TerrainID; // terrain generator
extern GLuint OccupancyID; // second stage to terrain, sets occupancy and stuff
extern GLuint UpdatesID; // updater
extern GLuint ResetID; // index occupancy resetter

// unloaded chunk ID
const uint unloaded = 0xFFFFFFFFu; // flag for if chunk isn't loaded

// enum flags for chunk state
const uint unstored = 4u; // unloaded
const uint stored = 3u; // loaded
const uint empty = 2u; // to be unloaded
const uint full = 1u; // to be unloaded
const uint generating = 0u; // currently generating

// chunk timing average helpers
double countTime = 0.0; // total time spent generating chunks
double genCount = 0.0; // amount of chunks generated

// gets flag of a slot
uint getFlag(uint slot) {
    return chunkData[viewChunks+slot]; // chunkData stores slots and flags, slots first
}

// sets flag of a slot
void setFlag(uint slot, uint flag) {
    chunkData[viewChunks+slot] = flag;
}

// gets slot at a chunk index
uint getSlot(uint cIndex) {
    return chunkData[cIndex]; // chunkData stores slots and flags, slots first
}

// sets flag of a slot
void setSlot(uint cIndex, uint slot) {
    chunkData[cIndex] = slot;
}

// gets position, rounded down to chunk
vec3 getChunkPos(vec3 p) {
    float cs = (float)chunkSize;
    vec3 cp = multiply_f3xf(floor_f3(multiply_f3xf(p, 1.0f/cs)),cs);
    return cp;
}

// gets local position within view, wrapped
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
    // set all indexes to unloaded
    glUseProgram(ResetID);
    glDispatchCompute((viewSize+3)/4,(viewSize+3)/4,(viewSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    createFenceGPU();
}

void getChunk(vec3 cPos, uint slot, uint cIndex) {
    // need to check if saved before loading it
    // stage 1, generate terrain, and get occupancy
    glUseProgram(TerrainID);
    shaderSetVec3(TerrainID, "cPos", cPos); // set position
    shaderSetUint(TerrainID, "slot", slot); // set slot

    glDispatchCompute((chunkSize+3)/4,(chunkSize+3)/4,(chunkSize+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void updateOccupancy(uint slot, uint cIndex) {
    // upon this finishing, set slot and clean occupancy
    glUseProgram(OccupancyID);
    shaderSetUint(OccupancyID, "cIndex", cIndex); // set position
    shaderSetUint(OccupancyID, "slot", slot); // set slot

    glDispatchCompute(1,1,1);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

// generates chunk at global chunk position, automatically puts it at nearest unloaded chunk index.
void generateChunk(vec3 cPos) {
    // time generation
    struct timespec start, end;
    timespec_get(&start, TIME_UTC);

    // get current chunk
    uvec3 lp = getLocalPos(subtract_f3(cPos, worldPos));
    uint cIndex = posToChunkIndex(lp);

    // make sure old chunk gets overwritten
    if (getSlot(cIndex) != unloaded) {
        // essentially gets the slot that that chunk index was assigned
        setFlag(getSlot(cIndex),unstored); // empty old slot this chunk was filling
        setSlot(cIndex,unloaded); // assume chunk is unloaded for now
    }

    // find an unloaded chunk, and go there
    uint slot = unloaded; // if slot stays unloaded, don't generate
    uint hash = hash_uint(cIndex); // hash for easier search
    for (uint i = 0u; i < viewChunks/cut; i++) {
        uint index = (i+hash)%(viewChunks/cut); // modulate within alloted space
        if (getFlag(index) == unstored) { // if unloaded chunk found, set slot.
            slot = index;
            setFlag(index,generating); // flag chunk as generating
            //printf("%u steps required for chunk.\n", i);
            break;
        }
    } if (slot == unloaded) return;  // if no space found, do nothing

    // generates chunk, which also checks if chunk is full and sets its slot
    
    getChunk(cPos, slot, cIndex);
    updateOccupancy(slot, cIndex);
    //createFenceGPU();

    // if chunk stored, set its slot
    //if (getFlag(slot) == stored) setSlot(cIndex,slot);

    // timer again
    timespec_get(&end, TIME_UTC);
    double duration = (end.tv_sec - start.tv_sec) + 
                      (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    countTime += duration;
    genCount += 1.0;
    printf("Current chunk took %f seconds, for an average of %f.\r", duration, countTime/genCount);
}

// updates chunk at position
void updateChunk(vec3 target, vec3 cPos, int click, uint type, float size, uint material) {
    // get current chunk
    uvec3 lp = getLocalPos(subtract_f3(cPos,worldPos));
    //printf("Update lp: (%u,%u,%u), ",lp.x,lp.y,lp.z);
    uint cIndex = posToChunkIndex(lp);

    uint slot = unloaded;
    if (getSlot(cIndex) != unloaded) { // get old chunk slot to edit if loaded
        slot = getSlot(cIndex);
        setSlot(cIndex, unloaded);
    }

    else {
    uint hash = hash_uint(cIndex); // hash for easier search
    for (uint i = 0u; i < viewChunks/cut; i++) { // otherwise find unloaded slot for one
        uint index = (i+hash)%(viewChunks/cut); // modulate within alloted space
        if (getFlag(index) == unstored) { // if unloaded chunk found, set slot.
            slot = index;

            getChunk(add_f3xf(cPos, center*2), slot, cIndex); // wtaf why do I need to add entire view of offset here
            //createFenceGPU(); // do need fence here because I can't dispatch next one without waiting for completion
            
            //printf("Chunk gotten flag: %u\n",chunkData[viewChunks]);
            //printf("Needed to find slot for chunk, ");
            break;
        }
    } if (slot == unloaded) return; // do nothing if no slot found
    }

    //setSlot(cIndex, unloaded); // unset slot because getChunk could set it.
    setFlag(slot, generating); // flag chunk as generating

    //printf("Using slot %u at %u chunk index.\n",slot, cIndex);

    //generates chunk, which also checks if chunk is full and sets its slot
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
    
    updateOccupancy(slot, cIndex);

    createFenceGPU(); // do need fence here because I can't dispatch next one without waiting for completion


    //if (getFlag(slot) == stored) setSlot(cIndex,slot);
 
    //else setFlag(slot, unstored); // if chunk not filled keep found slot unloaded
}

// need function for the first pass upon loading where you assign a index to every chunk position (will do grid with posToIndex function)
void genSpawnChunks() {
    //float time = glfwGetTime();
    // early out if incorrect initialization
    for (uint i = 0u; i < viewChunks; i--) {
        if (getSlot(i) != unloaded) {
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
}

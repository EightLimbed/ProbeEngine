// setting and getting helper functions
#pragma once
#include <Engine/types.h>

// world specs
extern const uint chunkSize;
extern const uint viewSize; // view size in chunks
extern const uint viewChunks;
extern const uint chunkProbes;
extern const float center;
extern vec3 worldPos; // position of world, for local positioning
const float dMin = -1.41421356237; // important to allow negative sdf distance data encoding.

// collision stuff
extern const uint simSize; // amount of chunks in simulation distance
extern const uint simProbes;
extern const uint simFidelity; // amount to cut collision buffer detail by in each axis

// pointer to index data
extern uint* chunkData;

// unloaded chunk ID
extern const uint unloaded; // flag for if chunk isn't loaded

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
    uvec3 lp = vec3_to_uvec3(glsl_modf3xf(add_f3(floor_f3(p),worldPos), (float)chunkSize*viewSize));
    return lp;
}

// gets index in index data
uint posToChunkIndex(uvec3 lp) {
    uvec3 cp = divide_u3xu(lp, chunkSize);
    return cp.x+viewSize*(cp.y+viewSize*cp.z);
}

// flat index for colliderData, uses local position
uint posToCollisionIndex(uvec3 lp) {
    uvec3 vp = divide_u3xu(lp,simFidelity);
    uint full = simSize/simFidelity;
    return vp.x+full*(vp.y+full*vp.z);
}
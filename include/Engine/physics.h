#pragma once
#include "glad/glad.h"
#include <Engine/types.h>
#include <Engine/shaders.h>
#include <Engine/setget.h>

// shaders
extern GLuint ColliderID; // terrain generator

// surface data
extern const uint simSize; // amount of chunks in simulation distance
extern const uint simProbes;
extern const uint simFidelity; // amount to cut collision buffer detail by in each axis

extern uint* colliderData; // lower res sdf data for collisions

// world stuff
extern vec3 worldPos; // position of world, for local positioning

// dispatches compute shader that gets 
void updatecolliderData() {
    //printf("Regenerated Collider.\n");
    glUseProgram(ColliderID);
    shaderSetVec3(ColliderID, "worldPos", worldPos);
    glDispatchCompute(((simSize+simFidelity-1)/simFidelity+3)/4,((simSize+simFidelity-1)/simFidelity+3)/4,((simSize+simFidelity-1)/simFidelity+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

float getColliderDist(vec3 p) {
    uvec3 lp = getLocalPos(subtract_f3(add_f3xf(p,center), worldPos));
    if (chunkData[posToChunkIndex(lp)]==unloaded) return 1.0; // safety

    uint index = posToCollisionIndex(lp);
    
    uint uintIndex = index >> 2u; // divide by 4
    uint bitShift = (index & 3u) * 8u; // shift amount

    uint packed = (colliderData[uintIndex] >> bitShift) & 0xFFu;
    
    return (float)packed/255.0f*(float)chunkSize+dMin;
}

// gets collider dist for collisions
float smoothColliderDist(vec3 p) {
    float s = (float)simFidelity;
    vec3 f = multiply_f3xf(glsl_modf3xf(p,s), 1.0/s);

    // get all probes
    float d000 = getColliderDist(add_f3(p, (vec3){0,0,0}));
    float d100 = getColliderDist(add_f3(p, (vec3){s,0,0}));
    float d010 = getColliderDist(add_f3(p, (vec3){0,s,0}));
    float d110 = getColliderDist(add_f3(p, (vec3){s,s,0}));
    float d001 = getColliderDist(add_f3(p, (vec3){0,0,s}));
    float d101 = getColliderDist(add_f3(p, (vec3){s,0,s}));
    float d011 = getColliderDist(add_f3(p, (vec3){0,s,s}));
    float d111 = getColliderDist(add_f3(p, (vec3){s,s,s}));

    // x interpolation
    float x00 = mixf(d000, d100, f.x);
    float x10 = mixf(d010, d110, f.x);
    float x01 = mixf(d001, d101, f.x);
    float x11 = mixf(d011, d111, f.x);

    // y interpolation
    float y0 = mixf(x00, x10, f.y);
    float y1 = mixf(x01, x11, f.y);

    // z interpolation
    float d = mixf(y0, y1, f.z);

    return d; // adjust for negative distances
}
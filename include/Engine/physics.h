#pragma once
#include "glad/glad.h"
#include <Engine/types.h>
#include <Engine/shaders.h>

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
    printf("Regenerated Collider.\n");
    glUseProgram(ColliderID);
    shaderSetVec3(ColliderID, "worldPos", worldPos);
    glDispatchCompute(((simSize+simFidelity-1)/simFidelity+3)/4,((simSize+simFidelity-1)/simFidelity+3)/4,((simSize+simFidelity-1)/simFidelity+3)/4);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
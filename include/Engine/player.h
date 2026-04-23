#include "glad/glad.h"
#include <Engine/types.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

typedef struct {
  // physical
  vec3 pos;
  vec3 dir;

  // stats
  float speed;

  // engine
  GLFWwindow *window;

} player;

void initializePlayer(player *p, vec3 pos, vec3 dir, float speed, GLFWwindow *window) {
  p->pos = pos;
  p->dir = dir;
  p->speed = 100.0;
  p->window = window;
}

void checkPlayer(player *p) {
  printf("Player position: (%.3f, %.3f, %.3f) \n", p->pos.x, p->pos.y, p->pos.z);
  printf("Player rotation: (%.3f, %.3f, %.3f) \n", p->dir.x, p->dir.y, p->dir.z);
}

void playerInputs(player *p, float deltaTime) {
  // forward
  if (glfwGetKey(p->window,GLFW_KEY_W)==GLFW_PRESS) {
    p->pos.x += p->speed*deltaTime;
    checkPlayer(p);
  }
  if (glfwGetKey(p->window,GLFW_KEY_S)==GLFW_PRESS) {
    p->pos.x -= p->speed*deltaTime;
    checkPlayer(p);
  }
  if (glfwGetKey(p->window,GLFW_KEY_A)==GLFW_PRESS) {
    p->pos.z += p->speed*deltaTime;
    checkPlayer(p);
  }
  if (glfwGetKey(p->window,GLFW_KEY_D)==GLFW_PRESS) {
    p->pos.z -= p->speed*deltaTime;
    checkPlayer(p);
  }
}
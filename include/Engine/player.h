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

} player;

void initializePlayer(player *p) {
  vec3 pos = {0.0, 0.0, 0.0};
  vec3 dir = {0.0, 0.0, 1.0};
  p->pos = pos;
  p->dir = dir;
  p->speed = 100.0;
}

void checkPlayer(player *p) {
  printf("Player position: (%.3f, %.3f, %.3f) \n", p->pos.x, p->pos.y, p->pos.z);
  printf("Player rotation: (%.3f, %.3f, %.3f) \n", p->dir.x, p->dir.y, p->dir.z);
}
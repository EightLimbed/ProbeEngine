#include "glad/glad.h"
#include <Engine/types.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>

typedef struct {
  // physical
  vec3 pos;
  vec3 dir;

  // stats
  float speed;

  // controls
  float sensitivity;
  double omx; // old mouse x
  double omy; // old mouse y
  float yaw;
  float pitch;

  // engine
  GLFWwindow *window;

} player;

void initializePlayer(player *p, vec3 pos, vec3 dir, float speed, float sensitivity, GLFWwindow *window) {
  p->pos = pos;
  p->dir = dir;
  p->speed = speed;
  p->window = window;
  p->sensitivity = sensitivity;
  p->pitch = asin(-p->dir.y);
  p->yaw = atan2(p->dir.x, p->dir.z);
}

void checkPlayer(player *p) {
  printf("Player position: (%.3f, %.3f, %.3f).\n", p->pos.x, p->pos.y, p->pos.z);
  printf("Player rotation: (%.3f, %.3f, %.3f).\n", p->dir.x, p->dir.y, p->dir.z);
}

// clamps player position within given bounds
void clampPlayer(player *p, vec3 A, vec3 B) {
  if (B.x<p->pos.x) p->pos.x = B.x; // max X
  if (B.y<p->pos.y) p->pos.y = B.y; // max Y
  if (B.z<p->pos.z) p->pos.z = B.z; // max Z
  if (A.x>p->pos.x) p->pos.x = A.x; // min X
  if (A.y>p->pos.y) p->pos.y = A.y; // min Y
  if (A.z>p->pos.z) p->pos.z = A.z; // min Z
}

void playerInputs(player *p, float deltaTime) {
  // gets forward direction
  vec3 forward = {p->dir.x, 0.0, p->dir.z};
  forward = normalize(forward);
  
  vec3 up = {0.0,1.0,0.0};
  vec3 right = normalize(cross(forward, up));

  // forward
  if (glfwGetKey(p->window,GLFW_KEY_W)==GLFW_PRESS) {
    p->pos.x += p->speed*deltaTime*forward.x;
    p->pos.y += p->speed*deltaTime*forward.y;
    p->pos.z += p->speed*deltaTime*forward.z;
  }
  // backward
  if (glfwGetKey(p->window,GLFW_KEY_S)==GLFW_PRESS) {
    p->pos.x -= p->speed*deltaTime*forward.x;
    p->pos.y -= p->speed*deltaTime*forward.y;
    p->pos.z -= p->speed*deltaTime*forward.z;
  }
  // right
  if (glfwGetKey(p->window,GLFW_KEY_D)==GLFW_PRESS) {
    p->pos.x -= p->speed*deltaTime*right.x;
    p->pos.y -= p->speed*deltaTime*right.y;
    p->pos.z -= p->speed*deltaTime*right.z;
  }
  // left
  if (glfwGetKey(p->window,GLFW_KEY_A)==GLFW_PRESS) {
    p->pos.x += p->speed*deltaTime*right.x;
    p->pos.y += p->speed*deltaTime*right.y;
    p->pos.z += p->speed*deltaTime*right.z;
  }
  // up
  if (glfwGetKey(p->window,GLFW_KEY_SPACE)==GLFW_PRESS) {
    p->pos.y += p->speed*deltaTime;
  }
  // down
  if (glfwGetKey(p->window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) {
    p->pos.y -= p->speed*deltaTime;
  }
}

void playerMouse(player *p) {
  // delta mouse movement
  double mousePosX;
  double mousePosY;
  double mouseDeltaX;
  double mouseDeltaY;

  if (glfwGetInputMode(p->window,GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
      glfwGetCursorPos(p->window, &mousePosX, &mousePosY);
      mouseDeltaX = mousePosX - p->omx;
      mouseDeltaY = mousePosY - p->omy;
      p->omx = mousePosX;
      p->omy = mousePosY;
  } else {
      mouseDeltaX = 0.0; // fix horizontal thing maybe
      mouseDeltaY = 0.0;
      p->omx = 0.0;
      p->omy = 0.0;
  }

  // update pitch and yaw
  mouseDeltaX *= p->sensitivity;
  mouseDeltaY *= p->sensitivity;
  p->yaw -= mouseDeltaX;
  p->pitch -= mouseDeltaY;

  // clamp pitch to prevent flipping
  if (p->pitch > 1.55f) p->pitch = 1.55f;
  if (p->pitch < -1.55f) p->pitch = -1.55f;

  // trigonometry to project to vector direction
  p->dir.x = cos(p->yaw) * cos(p->pitch);
  p->dir.y = sin(p->pitch);
  p->dir.z = sin(p->yaw) * cos(p->pitch);
}
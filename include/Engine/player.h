#include "glad/glad.h"
#include <Engine/types.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <Engine/physics.h>

typedef struct {
  // physical
  vec3 pos; // global position
  vec3 dir; // facing direction
  float gravity; // falling
  float height; // height of player
  float radius; // radius of player
  float terminal; // terminal velocity of player

  // stats
  float speed;

  // controls
  float sensitivity;
  double omx; // old mouse x
  double omy; // old mouse y
  float yaw;
  float pitch;

  // input
  int mousePress; // held
  int omp; // old mouse press
  int mouseClick; // clicked

  // engine
  GLFWwindow *window;

} player;

void initializePlayer(player *p, vec3 pos, vec3 dir, float height, float speed, float sensitivity, GLFWwindow *window) {
  p->pos = pos;
  p->dir = dir;
  p->speed = speed;
  p->height = height;
  p->radius = 1.8;
  p->window = window;
  p->sensitivity = sensitivity;
  p->pitch = asin(-p->dir.y);
  p->yaw = atan2(p->dir.x, p->dir.z);
  p->mouseClick = 0;
  p->omp = 0;
  p->mousePress = 0;
  p->gravity = 0;
  p->terminal = 220.0;
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
  //printf("Dist to Surface: %f\r", smoothColliderDist(p->pos));
  // gets forward direction
  vec3 forward = {p->dir.x, 0.0, p->dir.z};
  forward = normalize(forward);
  
  vec3 up = {0.0,1.0,0.0};
  vec3 right = normalize(cross(forward, up));
  //printf("Normal: (%f,%f,%f)\n",normal.x,normal.y,normal.z);

  // forward
  if (glfwGetKey(p->window,GLFW_KEY_W)==GLFW_PRESS) {
    p->pos = add_f3(p->pos, mult_f3xf(forward, p->speed*deltaTime));
  }

  // backward
  if (glfwGetKey(p->window,GLFW_KEY_S)==GLFW_PRESS) {
    p->pos = sub_f3(p->pos, mult_f3xf(forward, p->speed*deltaTime));
  }

  // left
  if (glfwGetKey(p->window,GLFW_KEY_A)==GLFW_PRESS) {
    p->pos = add_f3(p->pos, mult_f3xf(right, p->speed*deltaTime));
  }

  // right
  if (glfwGetKey(p->window,GLFW_KEY_D)==GLFW_PRESS) {
    p->pos = sub_f3(p->pos, mult_f3xf(right, p->speed*deltaTime));
  }

  // up
  if (glfwGetKey(p->window,GLFW_KEY_SPACE)==GLFW_PRESS) {
    //p->pos.y += p->speed*deltaTime*5.0;
    p->gravity = -100.0;
    p->pos.y += 1.0;
  }

  // handle gravity
  p->pos.y -= p->gravity*deltaTime;

  // down
  if (glfwGetKey(p->window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) {
    p->pos.y -= p->speed*deltaTime*5.0;
  }

  // teleport
  if (glfwGetKey(p->window,GLFW_KEY_P)==GLFW_PRESS) {
    p->pos = (vec3){128.0,0.0,128.0};
  }
}

void playerPhysics(player *p) {
  // contact points, 3 spheres stacked
  vec3 colliders[3]={{0.0,-p->height,0.0},{0.0,-p->height/2,0.0},{0.0,0.0,0.0}};

    // checks all spheres and resolves their collisions
  for (int i = 0; i<3; i++) {
    vec3 nPos = add_f3(p->pos, colliders[i]);
    float contact = smoothColliderDist(nPos);
    vec3 n = getNormal(nPos);

    if (contact<p->radius) {
      p->gravity = 0.0;
      p->pos = add_f3(p->pos,mult_f3xf(n, fabs(contact-p->radius))); // colliding
    }
  }

  // handle gravity
  p->gravity = minf(p->terminal,p->gravity+9.8);
}

void playerMouse(player *p) {
  // mouse buttons
  int left = glfwGetMouseButton(p->window, GLFW_MOUSE_BUTTON_LEFT);
  int right = glfwGetMouseButton(p->window, GLFW_MOUSE_BUTTON_RIGHT);
  p->mousePress = left-right; // -1 for right +1 for left
  if (p->omp == p->mousePress) p->mouseClick = 0; // checks if change in mouse
  else p->mouseClick = p->mousePress; // sets to change if there is
  p->omp = p->mousePress;

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
#pragma once

#include <math.h>

typedef struct {
  float x;
  float y;
  float z;
} vec3;

vec3 add_f3(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.x+b.x;
  c.y = a.y+b.y;
  c.z = a.z+b.z;
  return c;
}

vec3 multiply_f3xf3(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.x*b.x;
  c.y = a.y*b.y;
  c.z = a.z*b.z;
  return c;
}

vec3 multiply_f3xf(vec3 a, float f) {
  vec3 c;
  c.x = a.x*f;
  c.y = a.y*f;
  c.z = a.z*f;
  return c;
}

vec3 cross(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.y*b.z-a.z*b.y;
  c.y = a.z*b.x-a.x*b.z;
  c.z = a.x*b.y-a.y*b.x;
  return c;
}

vec3 normalize(vec3 a) {
  float len = sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);
  vec3 c;
  c.x = a.x/len;
  c.y = a.y/len;
  c.z = a.z/len;
  return c;
}
#pragma once
#define uint unsigned int
#include <math.h>

typedef struct {
  float x,y,z;
} vec3;

typedef struct {
  uint x,y,z;
} uvec3;

typedef struct {
  int x,y,z;
} ivec3;

typedef struct {
  uint x,y;
} uvec2;

vec3 add_f3(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.x+b.x;
  c.y = a.y+b.y;
  c.z = a.z+b.z;
  return c;
}

vec3 add_f3xf(vec3 a, float b) {
  vec3 c;
  c.x = a.x+b;
  c.y = a.y+b;
  c.z = a.z+b;
  return c;
}

vec3 subtract_f3(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.x-b.x;
  c.y = a.y-b.y;
  c.z = a.z-b.z;
  return c;
}

vec3 multiply_f3xf3(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.x*b.x;
  c.y = a.y*b.y;
  c.z = a.z*b.z;
  return c;
}

uvec3 divide_u3xu(uvec3 a, uint b) {
  uvec3 c;
  c.x = a.x/b;
  c.y = a.y/b;
  c.z = a.z/b;
  return c;
}

// glsl works differently for negative numbers
double glsl_mod(double x, double y) {
    return x - y * floor(x / y);
}

vec3 mod_f3xf(vec3 a, float b) {
  vec3 c;
  c.x = glsl_mod(a.x,b);
  c.y = glsl_mod(a.y,b);
  c.z = glsl_mod(a.z,b);
  return c;
}


vec3 multiply_f3xf(vec3 a, float f) {
  vec3 c;
  c.x = a.x*f;
  c.y = a.y*f;
  c.z = a.z*f;
  return c;
}

vec3 floor_f3(vec3 a) {
  vec3 c;
  c.x = floor(a.x);
  c.y = floor(a.y);
  c.z = floor(a.z);
  return c;
}

vec3 abs_f3(vec3 a) {
  vec3 c;
  c.x = fabsf(a.x);
  c.y = fabsf(a.y);
  c.z = fabsf(a.z);
  return c;
}

int equal_f3(vec3 a, vec3 b) {
  int equal = 1;
  if (a.x != b.x) equal = 0;
  if (a.y != b.y) equal = 0;
  if (a.z != b.z) equal = 0;
  return equal;
}

uvec3 vec3_to_uvec3(vec3 a) {
  uvec3 c;
  c.x = (uint)a.x;
  c.y = (uint)a.y;
  c.z = (uint)a.z;
  return c;
}

vec3 ivec3_to_vec3(ivec3 a) {
  vec3 c;
  c.x = (float)a.x;
  c.y = (float)a.y;
  c.z = (float)a.z;
  return c;
}

vec3 uvec3_to_vec3(uvec3 a) {
  vec3 c;
  c.x = (float)a.x;
  c.y = (float)a.y;
  c.z = (float)a.z;
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

int mini(int a, int b) {
  return (a<b) ? a : b;
}

int maxi(int a, int b) {
  return (a>b) ? a : b;
}

int stepi(int a, int b) {
  return (a<=b) ? 0 : 1;
}

uint hash_uint(uint x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}
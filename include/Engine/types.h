typedef struct {
  float x;
  float y;
  float z;
} vec3;

vec3 cross(vec3 a, vec3 b) {
  vec3 c;
  c.x = a.y*b.z-a.z*b.y;
  c.y = a.z*b.x-a.x*b.z;
  c.z = a.x*b.y-a.y*b.x;
  return c;
}
layout(std140, set = 0, binding = 1) uniform Camera {
  vec4 ScreenSize;
  vec3 camPos;
  vec3 camForward;
  vec3 camRight;
  vec3 camUp;
  float FOV_Tan;
};

layout(std140, set = 0, binding = 2) uniform LightingParameters {
  float lampStrength;
  int activeLighting;
};

layout(std140, set = 0, binding = 3) uniform SDFParameters {
  float time;
  float quality;
  float MinClip;
  float MaxClip;
  int activeSDF;
};

const int MAX_STEPS = 400;
const float PI = 3.14159;

struct rayHit {
  int pixelID;
  vec3 position;
  vec3 normal;
  float depth;
} hit;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D screen;

vec3 slerp(vec3 p1, vec3 p2, float t) {
  return cos((1 - t) * PI / 2) * p1 + sin(t * PI / 2) * p2;
}

vec3 GetViewDir(ivec2 id) {
  // normalized [-1,1]
  vec2 uv = (vec2(id) / ScreenSize.xy) * 2.0 - 1.0;
  uv.x *= ScreenSize.x / ScreenSize.y;
  // Sampling a Sphere
  vec3 rayDir = normalize(uv.x * camRight * FOV_Tan + uv.y * camUp * FOV_Tan +
                          camForward);
  return rayDir;
}

float fade(float t) {
  // return t;
  // return t * t * (3 - 2 * t);
  return t * t * t * (t * (t * 6 - 15) + 10);
}

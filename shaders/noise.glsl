uniform mat4 CamToWorld;
uniform float time;
uniform float lampDist;
uniform float lampStrength;
uniform float fov;
uniform float minDist = 0.001;
uniform float scalarDist;
uniform vec3 camPos;
uniform vec4 ScreenSize;
uniform int activeSDF;
uniform int activeLighting;
uniform bool isAOActive;

const int MAX_STEPS = 800;
const float PI = 3.14159;
const int SDF_Frac1 = 2;

struct rayHit {
  vec4 posDist;
};

struct Light {
  vec3 pos, color;
  float intensity, range;
};

struct SDFObj {
  vec3 position;
  vec3 scale;
  vec3 rotation;

  uint type;
  vec2 param;
};

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, binding = 0) uniform image2D Result;

layout(std430, binding = 0) buffer HitBufferRW
{
    rayHit hitBufferRW[];
};

layout(std430, binding = 1) readonly buffer HitBuffer
{
    rayHit hitBuffer[];
};

vec3 slerp(vec3 p1, vec3 p2, float t) {
  return cos((1 - t) * PI / 2) * p1 + sin(t * PI / 2) * p2;
}

float fade(float t) {
  // return t;
  // return t * t * (3 - 2 * t);
  return t * t * t * (t * (t * 6 - 15) + 10);
}

float RNGF(in vec3 pos) {
  vec3 hash = vec3(1993.7, 127.89, 77.41);
  float ret = dot(hash, pos);
  return fract(sin(ret) * 314159.865);
}

vec3 RNGNorm(in vec3 pos) {
  vec3 hashx = vec3(971.23, 231.67, 753.91);
  vec3 hashy = vec3(421.38, 882.19, 1193.57);

  float u = fract(sin(dot(pos, hashx)) * 4375.5453);
  float v = fract(sin(dot(pos, hashy)) * 4375.5453);

  float theta = 2 * 3.14159 * u;
  float z = 1.0 - 2.0 * v;
  float r = sqrt(1 - z);

  return vec3(r * cos(theta), r * sin(theta), v);
}

float valueNoise(vec3 pos) {
  vec3 i = floor(pos);
  vec3 f = fract(pos);

  vec3 u = vec3(fade(f.x), fade(f.y), fade(f.z));

  float g000 = RNGF(i + vec3(0, 0, 0));
  float g100 = RNGF(i + vec3(1, 0, 0));
  float g010 = RNGF(i + vec3(0, 1, 0));
  float g110 = RNGF(i + vec3(1, 1, 0));
  float g001 = RNGF(i + vec3(0, 0, 1));
  float g101 = RNGF(i + vec3(1, 0, 1));
  float g011 = RNGF(i + vec3(0, 1, 1));
  float g111 = RNGF(i + vec3(1, 1, 1));

  float nx00 = mix(g000, g100, u.x);
  float nx10 = mix(g010, g110, u.x);
  float nx01 = mix(g001, g101, u.x);
  float nx11 = mix(g011, g111, u.x);

  float nxy0 = mix(nx00, nx10, u.y);
  float nxy1 = mix(nx01, nx11, u.y);

  float nxyz = mix(nxy0, nxy1, u.z);

  return nxyz;
}

/*
float noiseFUNCY(vec3 p)
{
  float zAxis = texture(noiseTex, p.xy / 512).r;
  float yAxis = texture(noiseTex, p.yz / 512).r;
  float xAxis = texture(noiseTex, p.zx / 512).r;

  float ret = zAxis + yAxis + xAxis;

  return ret / 3;
}
*/

float Perlin(vec3 pos) {
  vec3 i = floor(pos);
  vec3 f = fract(pos);

  vec3 u = vec3(fade(f.x), fade(f.y), fade(f.z));

  vec3 g000 = RNGNorm(i + vec3(0, 0, 0));
  vec3 g100 = RNGNorm(i + vec3(1, 0, 0));
  vec3 g010 = RNGNorm(i + vec3(0, 1, 0));
  vec3 g110 = RNGNorm(i + vec3(1, 1, 0));
  vec3 g001 = RNGNorm(i + vec3(0, 0, 1));
  vec3 g101 = RNGNorm(i + vec3(1, 0, 1));
  vec3 g011 = RNGNorm(i + vec3(0, 1, 1));
  vec3 g111 = RNGNorm(i + vec3(1, 1, 1));

  vec3 p000 = f - vec3(0, 0, 0);
  vec3 p100 = f - vec3(1, 0, 0);
  vec3 p010 = f - vec3(0, 1, 0);
  vec3 p110 = f - vec3(1, 1, 0);
  vec3 p001 = f - vec3(0, 0, 1);
  vec3 p101 = f - vec3(1, 0, 1);
  vec3 p011 = f - vec3(0, 1, 1);
  vec3 p111 = f - vec3(1, 1, 1);

  float n000 = dot(g000, p000);
  float n100 = dot(g100, p100);
  float n010 = dot(g010, p010);
  float n110 = dot(g110, p110);
  float n001 = dot(g001, p001);
  float n101 = dot(g101, p101);
  float n011 = dot(g011, p011);
  float n111 = dot(g111, p111);

  float nx00 = mix(n000, n100, u.x); // lerp = mix.
  float nx10 = mix(n010, n110, u.x);
  float nx01 = mix(n001, n101, u.x);
  float nx11 = mix(n011, n111, u.x);

  float nxy0 = mix(nx00, nx10, u.y);
  float nxy1 = mix(nx01, nx11, u.y);

  float nxyz = mix(nxy0, nxy1, u.z);

  return nxyz;
}

/*
float SampleNoise3D(vec3 pos) {
  float noiseX = texture(noiseTex, pos.yz * 0.1).r;
  float noiseY = texture(noiseTex, pos.xz * 0.1).r;
  float noiseZ = texture(noiseTex, pos.xy * 0.1).r;

  return (noiseX + noiseY + noiseZ) * .3333333;
}
*/

vec3 RNGVec(in vec3 pos) {
  vec3 hashx = vec3(971.23, 231.67, 753.91);
  vec3 hashy = vec3(421.38, 882.19, 1193.57);
  vec3 hashz = vec3(362.15, 442.51, 953.15);

  float u = fract(sin(dot(pos, hashx)) * 4375.5453);
  float v = fract(sin(dot(pos, hashy)) * 4375.5453);
  float w = fract(sin(dot(pos, hashz)) * 4375.5453);

  return vec3(u, v, w);
}

mat3 RNGMatrix(in vec3 pos)
{
    mat3 primeMat = mat3(
    17.23, 53.87, 101.41,
    197.19, 263.56, 347.92,
    419.77, 521.33, 607.11);
    mat2x3 rand;
    rand[0] = primeMat * pos;
    rand[1] = primeMat * pos + 752;

    rand = mat2x3(sin(rand[0]), sin(rand[1]));
    rand *= 564.53;
    rand[0] = fract(rand[0] * 564.53) * 2 - 1;
    rand[1] = fract(rand[1] * 564.53) * 2 - 1;

    return mat3(rand[0].x, rand[0].y, rand[0].z,
        rand[0].y, rand[1].x, rand[1].y,
        rand[0].z, rand[1].y, rand[1].z);
}

float matrixNoise(vec3 pos)
{
    vec3 f = fract(pos); // frac = fract.
    vec3 i = floor(pos);

    float dots[8];

    vec3 u = f * f * f * (f * (f * 6 - 15) + 10);
    //vec3 u = f * f * (3.0 - 2.0 * f);

    mat3 m;
    vec3 n;

    for (int x = 0; x < 2; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            for (int z = 0; z < 2; z++)
            {
                //creates a random symetrical matrix
                m = RNGMatrix(i + vec3(x, y, z));
                //dots our local positition value with a matrix transformed
                n = RNGNorm(i + vec3(x, y, z));

                dots[x + y * 2 + z * 4] = dot(n, m * (pos - i  - vec3(x, y, z)));
            }
        }
    }

    dots[0] = mix(dots[0], dots[1], u.x);
    dots[1] = mix(dots[2], dots[3], u.x);
    dots[2] = mix(dots[4], dots[5], u.x);
    dots[3] = mix(dots[6], dots[7], u.x);

    dots[0] = mix(dots[0], dots[1], u.y);
    dots[1] = mix(dots[2], dots[3], u.y);

    dots[0] = mix(dots[0], dots[1], u.z);

    return dots[0];
}

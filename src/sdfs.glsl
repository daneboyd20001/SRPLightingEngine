uniform vec2 resolution;
uniform float time;
uniform vec3 camPos;
uniform vec3 camForward;
uniform vec3 camRight;
uniform vec3 camUp;
uniform int activeSDF;
uniform int activeLighting;
uniform float lampDist;
uniform sampler2D noiseTex;
uniform float lampStrength;
uniform float fov;
uniform float minDist = 0.001;
uniform float scalarDist;

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

float SampleNoise3D(vec3 pos) {
  float noiseX = texture(noiseTex, pos.yz * 0.1).r;
  float noiseY = texture(noiseTex, pos.xz * 0.1).r;
  float noiseZ = texture(noiseTex, pos.xy * 0.1).r;

  return (noiseX + noiseY + noiseZ) * .3333333;
}

float SphereSDF(vec3 p) { return length(p) - 1.0; }
float PlaneSDF(vec3 p) { return length(p.xy) - 1.0; }
float AABB(vec3 p) {
  p = abs(p) - 1;
  return max(max(p.x, p.y), p.z);
}

float CrossSDF(vec3 p) {
  float s = 0.2;
  float da = max(abs(p.x), abs(p.y));
  float db = max(abs(p.y), abs(p.z));
  float dc = max(abs(p.z), abs(p.x));
  return min(da, min(db, dc)) - s;
}

float GyroidTorus(vec3 p) {
  float tileSize = 40.0;
  p = mod(p + 40.0, tileSize) - 0.5 * tileSize;
  float rt = 15.0;
  float rg = 8.0;
  float ws = clamp(sin(time / 10.0) * 0.5 + 0.5, 0.05, 0.95);
  p.xz = vec2(rt * atan(p.z, -p.x), length(p.xz) - rt);
  p.yz = vec2(rg * atan(p.z, -p.y), length(p.yz) - rg);
  return 0.6 * max(abs(dot(sin(p), cos(p.yzx))) - ws, abs(p.z) - 0.5 * PI);
}

float TwistySphere(vec3 p) {
  float angle = p.y * 2.0;
  float c = cos(angle);
  float s = sin(angle);
  vec3 q = vec3(c * p.x - s * p.z, p.y, s * p.x + c * p.z);
  return length(q) -
         (1.0 + 0.3 * sin(5.0 * q.x) * sin(5.0 * q.y) * sin(5.0 * q.z));
}

float DanesSDF(vec3 p0) {
  vec4 p = vec4(p0, 1.0);
  vec3 cVal = vec3(1.0, 1.0, 1.0);
  float scale = 1.0;
  for (int i = 0; i < 12; i++) {
    p.xyz = 2.0 * clamp(p.xyz, -cVal, cVal) - p.xyz;
    p *= max(0.1, 2.0 / dot(p.xyz, p.xyz));
    p.xyz = p.xyz * scale;
    p.w *= abs(scale);
  }
  return (2.0 - (length(p.xyz / p.w))) * 0.2;
}

float WierdTriangleSDF(vec3 p) {
  vec4 p0 = vec4(p, 1.0);
  for (int i = 0; i < 8; i++) {
    p0.xyz = mod(p.xyz - 1.0, 2.0) - 1.0;
    p0 *= 1.4 / dot(p.xyz, p.xyz);
  }

  return length(p0.xyz / p0.w) / .25;
}

float cubeSDF(vec4 cube, vec3 pos) {
  vec3 d = cube.xyz - pos;
  return max(max(abs(d.x) - cube.w, abs(d.y) - cube.w), abs(d.z) - cube.w);
}

float SDF1(vec3 p) {
  // m = 0; // default material

  vec3 mp = mod(p, 0.1); // periodic tiling
  mp.y = p.y + sin(p.x * 2.0 + time) * 0.25 + sin(p.z * 2.5 + time) * 0.25;

  float PI = 3.14159265;

  // first cube SDF
  vec3 pos1 = vec3(
      mp.x, mp.y + (sin(p.z * PI * 10.0) * sin(p.x * PI * 10.0)) * 0.025, 0.05);
  float s1 = cubeSDF(vec4(0.05, 0.05, 0.05, 0.025), pos1);

  // second cube SDF
  vec3 pos2 =
      vec3(0.05, mp.y + (sin(p.x * PI * 10.0) * -sin(p.z * PI * 10.0)) * 0.025,
           mp.z);
  float s2 = cubeSDF(vec4(0.05, 0.05, 0.05, 0.025), pos2);

  // m = (s1 < s2) ? 0 : 1;

  return min(s1, s2);
}

float SDF2(vec3 p0) {
  p0 /= 10;
  p0.xyz = fract((p0.xyz - 1.0) * 0.5) * 2.0 - 1.0;

  vec4 p = vec4(p0, 1.0);
  p = abs(p);

  if (p.x < p.z)
    p.xz = p.zx;
  if (p.z < p.y)
    p.zy = p.yz;
  if (p.y < p.x)
    p.yx = p.xy;

  for (int i = 0; i < 10; i++) {
    if (p.x < p.z)
      p.xz = p.zx;
    if (p.z < p.y)
      p.zy = p.yz;
    if (p.y < p.x)
      p.yx = p.xy;

    p.xyz = abs(p.xyz);
    float dotVal = dot(p.xyz, p.xyz);
    p.xyz *= 1.6 / clamp(dotVal, 0.6, 1.0);
    p.xyz -= vec3(0.7, 1.8, 0.5);
    p.xyz *= 1.2;
  }

  float m = 1.5;
  p.xyz -= clamp(p.xyz, -m, m);

  return (length(p.xyz) / p.w) / 100.0;
}

float SDF3(vec3 p) {
  const float TAUg = atan(1.0) * 8.0;

  for (int i = 0; i < 4; i++) {
    // p.xy = pmodg(p.xy, 10.)
    float ang_xy = atan(p.y, p.x);
    float seg_xy = TAUg / 10.0;
    float a_xy = (ang_xy - seg_xy * floor(ang_xy / seg_xy)) - 0.5 * seg_xy;
    float r_xy = length(p.xy);
    p.xy = r_xy * vec2(sin(a_xy), cos(a_xy));

    p.y -= 2.0;

    // p.yz = pmodg(p.yz, 12.)
    float ang_yz = atan(p.z, p.y);
    float seg_yz = TAUg / 12.0;
    float a_yz = (ang_yz - seg_yz * floor(ang_yz / seg_yz)) - 0.5 * seg_yz;
    float r_yz = length(p.yz);
    p.yz = r_yz * vec2(sin(a_yz), cos(a_yz));

    p.z -= 10.0;
  }

  vec3 n = normalize(vec3(13.0, 1.0, 7.0));
  return dot(abs(p), n) - 0.7;
}

float SDF4(vec3 p0) {
  p0 /= 6.0;
  vec4 p = vec4(p0, 1.0);
  vec3 cVal = vec3(1.0, 1.0, 1.0);
  float scale = 1.0;
  for (int i = 0; i < 12; i++) {
    p.xyz = 2.0 * clamp(p.xyz, -cVal, cVal) - p.xyz;
    p *= max(0.1, 2.0 / dot(p.xyz, p.xyz));
    p.xyz = p.xyz * scale;
    p.w *= abs(scale);
  }
  return ((length(p.xyz / p.w)));
}

float SDF5(vec3 p, float time) {
  vec2 xy = p.xy;
  vec2 xz = p.xz;

  float angleOffset = time * 0.2;
  float scaleBase = 0.95;
  float timeSin = sin(time / 1000.0) * 0.1;

  for (int i = 0; i < 5; i++) {
    float t = time + float(i);
    float aXY = angleOffset + float(i) * 0.5;
    float aXZ = timeSin + float(i) * 0.1;

    float cXY = cos(aXY);
    float sXY = sin(aXY);
    float cXZ = cos(aXZ);
    float sXZ = sin(aXZ);

    xy = vec2(xy.x * cXY - xy.y * sXY, xy.x * sXY + xy.y * cXY);
    xz = vec2(xz.x * cXZ - xz.y * sXZ, xz.x * sXZ + xz.y * cXZ);

    xy = abs(xy) - 0.1;

    xy += 0.05 * sin(vec2(xy.y, xy.x) * 5.0 + t);
    xz += 0.03 * sin(vec2(xz.y, xz.x) * 7.0 + 1.3 * time + float(i));

    float s = scaleBase + 0.1 * sin(t);
    xy *= s;
    xz *= s;
  }

  return (length(xy) - 0.1) * 0.25 + 0.01 * sin(length(xz) * 10.0 + time);
}

float SDF6(vec3 p) {
  float e;
  float R = length(p) + 0.01;
  float theta = atan(p.x, p.y);
  float phi = asin(p.z / R);
  p = vec3(log(R), theta, phi);
  e = p.y - 1.5;

  for (int S = 1; S < 256; S = S << 1) {
    float fS = float(S);
    e += sqrt(abs(dot(sin(p.xxx * fS), cos(p * fS)))) / fS;
  }

  return (time + e * R) * 0.1;
}

float NoiseSDF(vec3 p) {
  float dist = 0;

  float scale = 8;
  float s = scale;

  for (int i = 0; i < 4; i++) {
    scale = s / float(i);
    dist += valueNoise(p / s) * s;
  }
  dist /= 3;

  float temp = dist;

  s = 4;
  scale = s;
  for (int i = 0; i < 4; i++) {
    dist += Perlin((p + 154) / scale) * scale;
    scale /= 2;
  }

  return (temp + dist) / 4;
}

float orbitSDF(vec3 p, float time) {

  float e =
      0.3; // Eccentricity, between -1 and 1, describes how elliptical it is.
  float a1 = 10; // Orbit size.
  float a2 = 13;

  float tilt = (11 * PI) / 6; // Radians

  float r1 = 1;
  float r2 = 0.3;
  float r3 = 0.5;

  // float t = time - 0.8 * cos(time);
  float t1 = (a1 * (1 - e * e)) / (1 + e * cos(time));
  float t2 = (a2 * (1 - e * e)) / (1 + e * cos(time));

  vec3 orbitr1 = vec3(t1 * cos(time), t1 * sin(time), t1 * cos(time));
  vec3 orbitr2 = vec3(t2 * cos(time), t2 * sin(time), t2 * cos(time) * tilt);

  float centerObj = length(p) - r1;
  float obj1 = length(p - orbitr1) - r2;
  float obj2 = length(p - orbitr2) - r3;

  return min(centerObj, min(obj1, obj2));
}

float hunterSDF(vec3 p) {
  float r = 2 - cos(p.x) + cos(p.y) + cos(p.z);
  return r;
}

float SDF(vec3 p) {
  switch (activeSDF) {
  case 0:
    return GyroidTorus(p);
  case 1:
    return SphereSDF(p);
  case 2:
    return PlaneSDF(p);
  case 3:
    return CrossSDF(p);
  case 4:
    return WierdTriangleSDF(p);
  case 5:
    return TwistySphere(p);
  case 6:
    return DanesSDF(p);
  case 7:
    return SDF1(p);
  case 8:
    return SDF2(p);
  case 9:
    return SDF3(p);
  case 10:
    return SDF4(p);
  case 11:
    return SDF5(p, time);
  case 12:
    return SDF6(p);
  case 13:
    return AABB(p);
  case 14:
    return NoiseSDF(p);
  case 15:
    return orbitSDF(p, time);
  case 16:
    return hunterSDF(p);
  default:
    return SDF2(p);
  }
}

vec3 GetGradient(vec3 p) {
  float eps = .02;

  vec3 ex = vec3(eps, 0, 0);
  vec3 ey = vec3(0, eps, 0);
  vec3 ez = vec3(0, 0, eps);

  vec3 diff = vec3(SDF(p + ex) - SDF(p - ex), SDF(p + ey) - SDF(p - ey),
                   SDF(p + ez) - SDF(p - ez));

  return diff / (eps * 2);
}

vec3 GetNormal(vec3 p) {
  float eps = .1;

  vec3 ex = vec3(eps, 0, 0);
  vec3 ey = vec3(0, eps, 0);
  vec3 ez = vec3(0, 0, eps);

  vec3 diff = vec3(SDF(p + ex) - SDF(p - ex), SDF(p + ey) - SDF(p - ey),
                   SDF(p + ez) - SDF(p - ez));

  return normalize(diff);
}

vec3 GetTangent(vec3 p, float theta) {
  hit.normal = GetNormal(p);
  vec3 b1, b2;
  if (hit.normal.z < -.99999999) {
    b1 = vec3(0, -1, 0);
    b2 = vec3(-1, 0, 0);
  } else {
    float a = 1.0f / (1.0f + hit.normal.z);
    float b = -a * hit.normal.x * hit.normal.y;
    b1 = vec3(1.0f - hit.normal.x * hit.normal.x * a, b, -hit.normal.x);
    b2 = vec3(b, 1.0f - hit.normal.y * hit.normal.y * a, -hit.normal.y);
  }
  return b1 * sin(theta) + b2 * cos(theta);
}

mat3 GetHessian(vec3 p) {
  float eps = .01;
  vec3 ddx = (GetGradient(p + vec3(eps, 0, 0)) - GetGradient(p - vec3(eps, 0, 0))) / (2.0 * eps);
  vec3 ddy = (GetGradient(p + vec3(0, eps, 0)) - GetGradient(p - vec3(0, eps, 0))) / (2.0 * eps);
  vec3 ddz = (GetGradient(p + vec3(0, 0, eps)) - GetGradient(p - vec3(0, 0, eps))) / (2.0 * eps);

  mat3 H;
  H[0] = vec3(ddx.x, ddy.x, ddz.x);
  H[1] = vec3(ddx.y, ddy.y, ddz.y);
  H[2] = vec3(ddx.z, ddy.z, ddz.z);
  return H;
}

float GetLaplacian(vec3 p) {
  float eps = .01;
  float diff = SDF(p + vec3(eps, 0, 0)) + SDF(p - vec3(eps, 0, 0)) +
               SDF(p + vec3(0, eps, 0)) + SDF(p - vec3(0, eps, 0)) +
               SDF(p + vec3(0, 0, eps)) + SDF(p - vec3(0, 0, eps));
  return (diff - SDF(p) * 6) / (eps * eps);
}

vec3 GetSurfaceEmission(vec3 p) {
  vec3 emission = vec3(1.0);

  // Scale P by local Curvature
  p = p * 1.0 / (1 + GetLaplacian(p));
  vec3 col1 =
      vec3(1, .45, .1) * sin(time / 11) + vec3(.1, .85, 1) * cos(time / 11);
  vec3 col2 =
      vec3(.1, .1, .9) * sin(time / 23) + vec3(.76, .1, 1) * cos(time / 23);

  float t = (matrixNoise(p) + 1) / 2;
  col1 *= t;
  col2 *= (1 - t);
  // return col1 + col2;
  return GetGradient(p);
}

vec3 Lighting(float dist, ivec2 id) {
  vec3 rayDir = GetViewDir(id);

  vec3 emission = vec3(1.0);

  switch (activeLighting) {
  case 0: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));
    break;
  }
  case 1: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));

    float lambert = dot(hit.normal, -rayDir);
    float rimLighting = 1.0 - lambert;
    float fresnel = pow(1.0 - abs(dot(hit.normal, rayDir)), 4.0);

    emission *= rimLighting + fresnel;
    break;
  }
  case 2: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));

    float lambert = max(dot(hit.normal, -rayDir), 0.0);

    emission *= lambert * exp(-dist / lampStrength);
    break;
  }
  case 3: {
    emission *= exp(-dist / lampStrength);
    break;
  }
  case 4: {
    emission *= 1 - exp(-dist / lampStrength);
    break;
  }
  default: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));
    break;
  }
  }

  return emission;
}

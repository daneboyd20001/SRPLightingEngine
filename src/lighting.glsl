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
  float eps = .05;

  vec3 ex = vec3(eps, 0, 0);
  vec3 ey = vec3(0, eps, 0);
  vec3 ez = vec3(0, 0, eps);

  vec3 diff = vec3(SDF(p + ex) - SDF(p - ex), SDF(p + ey) - SDF(p - ey),
                   SDF(p + ez) - SDF(p - ez));

  return normalize(diff);
}

vec3 GetTangent(vec3 p, float theta) {
  vec3 normal = GetNormal(p);
  vec3 b1, b2;
  if (normal.z < -.99999999) {
    b1 = vec3(0, -1, 0);
    b2 = vec3(-1, 0, 0);
  } else {
    float a = 1.0f / (1.0f + normal.z);
    float b = -a * normal.x * normal.y;
    b1 = vec3(1.0f - normal.x * normal.x * a, b, -normal.x);
    b2 = vec3(b, 1.0f - normal.y * normal.y * a, -normal.y);
  }
  return b1 * sin(theta) + b2 * cos(theta);
}

mat3 GetHessian(vec3 p) {
  float eps = .01;
  vec3 ddx = GetGradient(p + vec3(eps, 0, 0)) -
             GetGradient(p - vec3(eps, 0, 0)) / (2 * eps);
  vec3 ddy = GetGradient(p + vec3(0, eps, 0)) -
             GetGradient(p - vec3(0, eps, 0)) / (2 * eps);
  vec3 ddz = GetGradient(p + vec3(0, 0, eps)) -
             GetGradient(p - vec3(0, 0, eps)) / (2 * eps);

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

float GetCurvature(vec3 p) { return GetLaplacian(p); }

vec3 GetSurfaceEmission(vec3 p) {
  vec3 emission = vec3(1.0);

  // Scale P by local Curvature
  p = p * 1.0 / (1 + GetCurvature(p));
  vec3 col1 =
      vec3(1, .45, .1) * sin(time / 11) + vec3(.1, .85, 1) * cos(time / 11);
  vec3 col2 =
      vec3(.1, .1, .9) * sin(time / 23) + vec3(.76, .1, 1) * cos(time / 23);

  float t = (Perlin(p) + 1) / 2;
  col1 *= t;
  col2 *= (1 - t);
  // return col1 + col2;
  return GetGradient(p);
}

vec3 Lighting(rayHit hit) {
  vec3 lightSource = normalize(vec3(sin(time), cos(time), 0.0));
  vec2 uv = (vec2(gl_FragCoord.xy)) / resolution * 2.0 - 1;
  uv.x *= resolution.x / resolution.y;
  // Sampling a Sphere
  vec3 rayDir = normalize(uv.x * camRight + uv.y * camUp + fov * camForward);

  vec3 pos = hit.posDist.xyz;
  float dist = hit.posDist.w;
  vec3 normal = GetNormal(pos);

  vec3 emission = vec3(1.0);

  switch (activeLighting) {
  case 0: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));
    break;
  }
  case 1: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));

    float lambert = dot(normal, -rayDir);
    float rimLighting = 1.0 - lambert;
    float fresnel = pow(1.0 - abs(dot(normal, rayDir)), 4.0);

    emission *= rimLighting + fresnel;
    break;
  }
  case 2: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));

    float lambert = max(dot(normal, -rayDir), 0.0);

    emission *= lambert * exp(-dist / lampStrength);
    break;
  }
  case 3: {
    emission *= exp(-dist / lampStrength);
    break;
  }
  case 4: {
    float t = exp(-dist / lampStrength);
    emission = t * emission + vec3(1.0 - t);
    break;
  }
  case 5: {
    emission = GetSurfaceEmission(pos + GetTangent(pos, time / 7));

    float c0 = 200 / 4;
    float c1 = 200 / 4; // exponential scalar
    float spread = (c0 * c0) / ((dist + c0) * (dist + c0));
    float absorption = exp2(-dist / c1);

    emission *= spread * absorption;

    // emission = lerp(emission, float3(.1, .1, .1), 1 - absorption);

    float sigma = .5;
    float sigma2 = sigma * sigma;

    float NdotL = max(dot(normal, lightSource), 0.0);
    float NdotV = max(dot(normal, -rayDir), 0.0);

    vec3 Lperp = normalize(lightSource - normal * NdotL);
    vec3 Vperp = normalize(-rayDir - normal * NdotV);
    float cosPhiDiff = clamp(dot(Lperp, Vperp), 0.0, 1.0);

    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));

    vec3 diffuse = emission * (NdotL * A + B * cosPhiDiff * max(NdotL, NdotV));

    float lambert = dot(normal, -rayDir);
    float rimLighting = 1 - lambert;
    float fresnel = pow(1 - abs(dot(normal, rayDir)), 4.0);

    emission *= (diffuse + rimLighting + fresnel);
    emission = mix(vec3(rimLighting), emission, absorption);
  }
  default: {
    emission *= lampStrength / ((dist + 1) * (dist + 1));
    break;
  }
  }

  return emission;
}

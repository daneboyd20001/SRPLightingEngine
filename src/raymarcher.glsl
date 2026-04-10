out vec4 FragColor;

void main() {
  // normalized [-1,1]
  vec2 uv = (vec2(gl_FragCoord.xy)) / resolution * 2.0 - 1;
  uv.x *= resolution.x / resolution.y;
  // Sampling a Sphere
  vec3 rayDir = normalize(uv.x * camRight + uv.y * camUp + fov * camForward);

  int i = 0;
  float dist = 0;
  float distToScene = 0;

  vec3 pos = camPos + rayDir * minDist;
  for (i = 0; i < MAX_STEPS; i++) {
    distToScene = SDF(pos) * scalarDist;

    pos += rayDir * distToScene;
    dist += max(distToScene, 0);

    if (abs(distToScene) < max(minDist * dist, minDist))
      break;
    if (abs(dist) > 200) {
      dist = 100000;
      break;
    }
  }
  dist = abs(dist);

  rayHit hitData;
  hitData.posDist = vec4(pos, dist);

  vec3 col = Lighting(hitData);

  FragColor = vec4(col, 1.0);
}

/*

    emission.x = matrixNoise(hit.posDist.xyz);
    emission.y = matrixNoise(hit.posDist.xyz - 654);
    emission.z = matrixNoise(hit.posDist.xyz + 123);


    //float sqrDist = 20 / hit.posDist.w / hit.posDist.w;
    float linDist = max(2 - hit.posDist.w, .5);
    linDist = pow(linDist, 4);
    emission *= linDist;

    vec3 purp = vec3(.1, 0, .1);
    purp *= sin(hit.posDist.w) + 1;
    purp *= 10 / (hit.posDist.w + 5);
    emission += purp;

*/

vec3 slerp(vec3 p1, vec3 p2, float t) {
  return cos((1 - t) * 3.14159 / 2) * p1 + sin(t * 3.14159 / 2) * p2;
}

vec3 RNGVec(in vec3 pos) {
  vec3 hashx = vec3(971.23, 231.67, 753.91);
  vec3 hashy = vec3(421.38, 882.19, 1193.57);
  vec3 hashz = vec3(362.15, 442.51, 953.15);

  float u = fract(sin(dot(pos, hashx)) * 4375.5453);
  float v = fract(sin(dot(pos, hashy)) * 4375.5453);
  float w = fract(sin(dot(pos, hashz)) * 4375.5453);

  return vec3(u, v, w);
}

/*
mat3 RNGMatrix(in vec3 pos)
{
    mat3 primeMat = mat3(
    17.23, 53.87, 101.41,
    197.19, 263.56, 347.92,
    419.77, 521.33, 607.11);
    mat2x3 rand;
    rand[0] = primeMat * pos;
    rand[1] = primeMat * pos + 752;

    rand = sin(rand);
    rand *= 564.53;
    rand = fract(rand * 564.53) * 2 - 1;

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

    for (int x = 0; x < 2; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            for (int z = 0; z < 2; z++)
            {
                //creates a random symetrical matrix
                m = RNGMatrix(i + vec3(x, y, z));
                //dots our local positition value with a matrix transformed
                vec3 localPos = pos - i - vec3(x, y, z);

                dots[x + y * 2 + z * 4] = dot(localPos, m * localPos);
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
*/


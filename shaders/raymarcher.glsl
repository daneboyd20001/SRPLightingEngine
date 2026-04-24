void main() {
  ivec2 id = ivec2(gl_GlobalInvocationID.xy);

  // Stop threads that are outside the screen boundaries
    if (id.x >= int(ScreenSize.x) || id.y >= int(ScreenSize.y)) return;

  // normalized [-1,1]
  vec2 uv = (vec2(gl_GlobalInvocationID.xy)) / ScreenSize.xy * 2.0 - 1.0;
  uv.x *= ScreenSize.z;
  // Sampling a Sphere
  vec3 rayDir = normalize(vec3(uv.x * fov, uv.y * fov, 1.0));
  rayDir = mat3(CamToWorld) * rayDir;

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
  hitBufferRW[id.x + id.y * int(ScreenSize.x)] = hitData;

  vec3 color = Lighting();
  imageStore(Result, id, vec4(color, 1.0));

  memoryBarrierImage();

  if (isAOActive)
    AmbientOcclusion();
}

void main() {
  ivec2 id = ivec2(gl_GlobalInvocationID.xy);

  vec3 rayDir = GetViewDir(id);

  // Cone tracing, how large is pixel relative to screen
  float cutoff = .001 * quality;
  float dist = MinClip, distToScene;
  vec3 pos = camPos + rayDir * dist;
  for (int i = 0; i < MAX_STEPS; i++) {
    distToScene = SDF(pos) * quality;
    dist += distToScene;
    // This should converge slightly better
    pos = rayDir * dist + camPos;

    // use ABS to better converge for fractal geometries
    if (abs(distToScene) < max(cutoff * dist, cutoff))
      break;
    if (dist > MaxClip)
      break;
  }

  hit.normal = GetNormal(pos);
  hit.depth = dist;
  hit.pixelID = id.x + id.y * int(ScreenSize.x);
  hit.position = pos;

  vec4 pixelColor =
      vec4(0.0, 0.0, 0.0, 1.0); // The color output if the ray hits nothing.
  if (dist < MaxClip)
    pixelColor = vec4(Lighting(dist, id), 1.0); // If the ray hits something.

  imageStore(screen, id, pixelColor);
}

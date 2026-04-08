out vec4 FragColor;

uniform float fov;
uniform float MIN_DIST;
uniform float scalarDist;

void main() {
    vec2 uv = (gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    vec3 rayDir = normalize(uv.x * camRight + uv.y * camUp + fov * camForward);
    
    vec3 pos = camPos;
    float dist = 0.0;
    float distToScene = 0.0;
    int steps = 0;

    for (int i = 0; i < MAX_STEPS; i++) {
        distToScene = map(pos) * scalarDist;
        pos += rayDir * distToScene;
        dist += max(distToScene, 0.0);
        
        if (abs(distToScene) < max(MIN_DIST, MIN_DIST * dist) || dist > MAX_CLIP) {
            steps = i;
            break;
        }
    }

    vec3 col = Lighting(pos, rayDir, dist);

    FragColor = vec4(col, 1.0);
}

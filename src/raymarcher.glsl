out vec4 FragColor;
void main() {
    vec2 uv = (gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    float fov = 1.0;
    vec3 rayDir = normalize(uv.x * camRight + uv.y * camUp + fov * camForward);
    
    vec3 pos = camPos;
    float dist = 0.0;
    float distToScene = 0.0;
    int steps = 0;

    for (int i = 0; i < MAX_STEPS; i++) {
        distToScene = map(pos);
        pos += rayDir * distToScene;
        dist += max(distToScene, 0.0);
        
        if (abs(distToScene) < MIN_DIST || dist > MAX_CLIP) {
            steps = i;
            break;
        }
    }

    vec3 col = CalculateLighting(pos, rayDir, dist);

    FragColor = vec4(col, 1.0);
}

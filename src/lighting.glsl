uniform float lampStrength;

vec3 GetNormal(vec3 p) {
    float eps = 0.05;
    vec2 e = vec2(eps, 0.0);
    return normalize(vec3(
        map(p + e.xyy) - map(p - e.xyy),
        map(p + e.yxy) - map(p - e.yxy),
        map(p + e.yyx) - map(p - e.yyx)
    ));
}

vec3 CalculateLighting(vec3 pos, vec3 rayDir, float dist) {
    vec3 normal = GetNormal(pos);

    vec3 background = vec3(0.3);

    vec3 finalLighting = vec3(1.0);

    switch (activeLighting) {
      case 0: {
                finalLighting *= lampStrength/((dist+1)*(dist+1));
                break;
              }
        case 1: {
            finalLighting *= lampStrength/((dist+1)*(dist+1));

            float lambert = dot(normal, -rayDir);
            float rimLighting = 1.0 - lambert;
            float fresnel = pow(1.0 - abs(dot(normal, rayDir)), 4.0);
            
            finalLighting *= rimLighting + fresnel;
            break;
        }
        case 2: {
            finalLighting *= lampStrength/((dist+1)*(dist+1));

            float lambert = max(dot(normal, -rayDir), 0.0);
            
            finalLighting *= lambert * exp(-dist/lampStrength);
            break;
        }
        case 3: {
            finalLighting *= exp(-dist/lampStrength);

            break;
        }
        case 4:{
            float t = exp(-dist/lampStrength);
            finalLighting = t * finalLighting + (1 - t) * background;
            break;
        }

        default: {
            finalLighting *= lampStrength/((dist+1)*(dist+1));

            break;
        }
    }

    return finalLighting;
}

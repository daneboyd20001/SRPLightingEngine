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
    vec3 bgColor = vec3(0.01, 0.01, 0.02);
    
    if (dist >= MAX_CLIP) {
        return bgColor;
    }

    vec3 normal = GetNormal(pos);
    
    vec3 lightPos = camPos + camRight * 0.5 - camUp * 0.5;
    vec3 lightDir = normalize(lightPos - pos);
    
    float noiseVal = SampleNoise3D(pos, normal);
    vec3 baseColor = vec3(0.8, 0.2, 0.9) * (0.8 + 0.2 * noiseVal);
    
    float trueDist = length(lightPos - pos);
    float attenuation = clamp(1.0 - (trueDist * trueDist) / (lanternRadius * lanternRadius), 0.0, 1.0);
    attenuation *= attenuation;
    vec3 attenuatedColor = baseColor * attenuation;

    float shadow = 1.0;
    vec3 shadowPos = pos + normal * 0.05;
    float t = 0.05;
    for(int j = 0; j < 30; j++) {
        float h = map(shadowPos + lightDir * t);
        if(h < 0.005) { shadow = 0.05; break; }
        shadow = min(shadow, 4.0 * h / t);
        t += clamp(h, 0.02, 0.25);
        if(t > length(lightPos - shadowPos)) break;
    }

    vec3 finalLighting = vec3(0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);

    switch (activeLighting) {
        case 0: {
            float sigma = 0.5;
            float sigma2 = sigma * sigma;
            float NdotV = max(dot(normal, -rayDir), 0.0);
            
            vec3 Lperp = normalize(lightDir - normal * NdotL);
            vec3 Vperp = normalize(-rayDir - normal * NdotV);
            float cosPhiDiff = clamp(dot(Lperp, Vperp), 0.0, 1.0);
            
            float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
            float B = 0.45 * (sigma2 / (sigma2 + 0.09));
            
            vec3 diffuse = attenuatedColor * (NdotL * A + B * cosPhiDiff * max(NdotL, NdotV));

            float lambert = max(dot(normal, -rayDir), 0.0);
            float rimLighting = 1.0 - lambert;
            float fresnel = pow(1.0 - abs(dot(normal, rayDir)), 4.0);
            
            finalLighting = diffuse + vec3(rimLighting * 0.15 * attenuation) + vec3(fresnel * 0.4 * attenuation);
            break;
        }
        case 1: {
            vec3 diffuse = attenuatedColor * NdotL;
            finalLighting = diffuse;
            break;
        }
        default: {
            finalLighting = attenuatedColor * NdotL;
            break;
        }
    }

    return finalLighting * shadow;
}

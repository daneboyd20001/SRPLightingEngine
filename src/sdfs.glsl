uniform vec2 resolution;
uniform float time;
uniform vec3 camPos;
uniform vec3 camForward;
uniform vec3 camRight;
uniform vec3 camUp;
uniform int activeSDF;
uniform int activeLighting;
uniform float lanternRadius;
uniform sampler2D noiseTex;

const int MAX_STEPS = 400;
const float MIN_DIST = 0.001;
const float MAX_CLIP = 100.0;
const float PI = 3.14159;

float SampleNoise3D(vec3 pos, vec3 normal) {
    vec3 blendWeights = abs(normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

    float noiseX = texture(noiseTex, pos.yz * 0.1).r;
    float noiseY = texture(noiseTex, pos.xz * 0.1).r;
    float noiseZ = texture(noiseTex, pos.xy * 0.1).r;

    return noiseX * blendWeights.x + noiseY * blendWeights.y + noiseZ * blendWeights.z;
}

float SphereSDF(vec3 p) { return length(p) - 1.0; }
float PlaneSDF(vec3 p) { return length(p.xy) - 1.0; }
float AABB(vec3 p)
{
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
    return length(q) - (1.0 + 0.3 * sin(5.0 * q.x) * sin(5.0 * q.y) * sin(5.0 * q.z));
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

float WierdTriangleSDF(vec3 p)
{
    vec4 p0 = vec4(p, 1.0);
    for (int i = 0; i < 8; i++)
    {
        p0.xyz = mod(p.xyz - 1.0, 2.0) - 1.0;
        p0 *= 1.4 / dot(p.xyz, p.xyz);
    }
        
    return length(p0.xyz / p0.w) / .25;
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

        xy = vec2(xy.x * cXY - xy.y * sXY,
                  xy.x * sXY + xy.y * cXY);
        xz = vec2(xz.x * cXZ - xz.y * sXZ,
                  xz.x * sXZ + xz.y * cXZ);
                  
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

float map(vec3 p) {
    switch (activeSDF) {
        case 0: return GyroidTorus(p);
        case 1: return SphereSDF(p);
        case 2: return PlaneSDF(p);
        case 3: return CrossSDF(p);
        case 4: return WierdTriangleSDF(p);
        case 5: return TwistySphere(p);
        case 6: return DanesSDF(p);
        case 7: return SDF4(p);
        case 8: return SDF5(p, time);
        case 9: return SDF6(p);
        case 10: return AABB(p);
        default: return GyroidTorus(p);
    }
}

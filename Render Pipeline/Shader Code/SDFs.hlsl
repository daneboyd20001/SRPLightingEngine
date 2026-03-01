
#include "Noise.hlsl"
struct SDFObj
{
    float3 position;
    float3 scale;
    float3 rotation;
    
    uint type;
    float2 param;
};

#define Pi 3.14159
uniform float Time;
static const uint SDF_SPHERE = 0;
static const uint SDF_Plane = 1;
static const uint SDF_AABB = 2;
static const uint SDF_Cross = 3;
static const uint SDF_GyroidTorus = 4;

float SphereSDF(in float3 p)
{
    return length(p) - 1;
}
float PlaneSDF(in float3 p)
{
    return length(p.xy) - 1;
}
float AABB(float3 p)
{
    p = abs(p) - 1;
    return max(max(p.x, p.y), p.z);

}

float Cross(float3 p)
{
    float s = 0.2; // size of the cross members
    float da = max(abs(p.x), abs(p.y));
    float db = max(abs(p.y), abs(p.z));
    float dc = max(abs(p.z), abs(p.x));
    return min(da, min(db, dc)) - s;
}

float GyroidTorus(float3 p)
{
    float tileSize = 40;
    
    // Repeat space in all directions
    p = fmod(p + 40, tileSize) - 0.5 * tileSize;

    float rt = 15.0;
    float rg = 8.0;
    float ws = clamp(sin(Time / 10) * 0.5 + 0.5, 0.05, 0.95);

    // Transformations for torus structure
    p.xz = float2(rt * atan2(p.z, -p.x), length(p.xz) - rt);
    p.yz = float2(rg * atan2(p.z, -p.y), length(p.yz) - rg);

    // Gyroid-torus combination
    return 0.6 * max(abs(dot(sin(p).xyz, cos(p).yzx)) - ws, abs(p.z) - 0.5 * 3.14159265);
}

static const int SDF_Frac1 = 2;

float DanesSDF(float3 p0)
{
    float4 p = float4(p0, 1);
    float3 cVal = float3(1., 1., 1.);
    float scale = 1;
    for (int i = 0; i < 12; i++)
    {
        p.xyz = 2.0 * clamp(p.xyz, -cVal, cVal) - p.xyz;
        p *= max(.1, 2 / dot(p.xyz, p.xyz));
        // Scale and offset by original point
        p.xyz = p.xyz * scale;
        p.w *= abs(scale);
    }
    
    return (2 - (length(p.xyz / p.w))) * .2f;
}
	
float SDF4(float3 p0)
{
    p0 /= 6;
    float4 p = float4(p0, 1);
    float3 cVal = float3(1., 1., 1.);
    float scale = 1;
    for (int i = 0; i < 12; i++)
    {
        p.xyz = 2.0 * clamp(p.xyz, -cVal, cVal) - p.xyz;
        p *= max(.1, 2 / dot(p.xyz, p.xyz));
        // Scale and offset by original point
        p.xyz = p.xyz * scale;
        p.w *= abs(scale);
    }
    
    return ((length(p.xyz / p.w)));
}
  // highly varied domain - take a look around
float WierdTriangleSDF(float3 p)
{
    float4 p0 = float4(p, 1.0);
    for (int i = 0; i < 8; i++)
    {
        p0.xyz = fmod(p.xyz - 1.0, 2.0) - 1.0;
        p0 *= 1.4 / dot(p.xyz, p.xyz);
    }
        
    return length(p0.xyz / p0.w) / .25;
}

float TwistySphere(float3 p)
{
    // twist along Y axis
    float angle = p.y * 2.0; // controls twist frequency
    float c = cos(angle);
    float s = sin(angle);
    float3 q = float3(c * p.x - s * p.z, p.y, s * p.x + c * p.z);

    // wavy radius
    float r = length(q) - (1.0 + 0.3 * sin(5.0 * q.x) * sin(5.0 * q.y) * sin(5.0 * q.z));

    return r;
}


float2x2 rotMatrix(float r)
{
    return float2x2(cos(r), sin(r), -sin(r), cos(r));
}

float SDF5(float3 p, float Time)
{
    
    float2 xy, xz;
    xy = p.xy;
    xz = p.xz;

    
    float d = 0.0;
    
    // Precompute sin/cos increments
    float angleOffset = Time * 0.2;
    float scaleBase = .95;

    [unroll]
    for (int i = 0; i < 7; i++)
    {
        float t = Time + float(i);

        // Precompute angles
        float aXY = angleOffset + i * 0.5;
        float aXZ = sin(Time / 1000) * 0.1 + i * 0.1;

        float cXY = cos(aXY);
        float sXY = sin(aXY);
        float cXZ = cos(aXZ);
        float sXZ = sin(aXZ);

        // Rotate XY plane
        xy = float2(xy.x * cXY - xy.y * sXY,
                    xy.x * sXY + xy.y * cXY);

        // Rotate XZ plane
        xz = float2(xz.x * cXZ - xz.y * sXZ,
                    xz.x * sXZ + xz.y * cXZ);

        // Mirror and subtract constant
        xy = abs(xy) - 0.1;

        // Wobble effect
        xy += 0.05 * sin(float2(xy.y, xy.x) * 5.0 + t);
        xz += 0.03 * sin(float2(xz.y, xz.x) * 7.0 + 1.3 * Time + i);

        // Non-uniform scaling
        float s = scaleBase + 0.1 * sin(t);
        xy *= s;
        xz *= s;
    }

    // Combine distances efficiently
    d = (length(xy) - 0.1) * 0.1 + 0.01 * sin(length(xz) * 10.0 + Time);

    return d;
}

float SDF6(float3 p)
{
    float e;
    
    float R = length(p) + .01f;
    float theta = atan2(p.x, p.y);
    float phi = asin(p.z / R);
    
    p = float3(
        log(R),
        theta,
        phi
    );
    
    e = p.y - 1.5f;
    for (int S = 1; S < 256; S = S << 1)
    {
        e += sqrt(abs(dot(sin(p.xxx * S), cos(p * S)))) / S;
    }
    
    return (Time + e * R) * .1f;
}

float NoiseSDF(float3 pos)
{
    float dist = 0;
    
    float scale = 8;
    float s = scale;
    
    for (int i = 0; i < 4; i++)
    {
        scale = s / float(i);
        dist += valueNoise(pos / s) * s;
    }
    dist /= 3;
    
    float temp = dist;
    
    s = 4;
    scale = s;
    for (i = 0; i < 4; i++)
    {
        dist += Perlin((pos + 154) / scale) * scale;
        scale /= 2;
    }
    
    return (temp + dist) / 4;
}

float SDF(float3 p)
{
    return SDF6(p);

}


float3 GetGradient(float3 p)
{
    float eps = .01;
    
    float3 ex = float3(eps, 0, 0);
    float3 ey = float3(0, eps, 0);
    float3 ez = float3(0, 0, eps);
    
    float3 diff = float3(
        SDF(p + ex) - SDF(p - ex),
        SDF(p + ey) - SDF(p - ey),
        SDF(p + ez) - SDF(p - ez)
    );
    
    return diff;
}

float3 GetNormal(float3 p)
{
    float eps = .01;
    
    float3 ex = float3(eps, 0, 0);
    float3 ey = float3(0, eps, 0);
    float3 ez = float3(0, 0, eps);
    
    float3 diff = float3(
        SDF(p + ex) - SDF(p - ex),
        SDF(p + ey) - SDF(p - ey),
        SDF(p + ez) - SDF(p - ez)
    );
    
    return normalize(diff);
}

float GetCurvature(float3 p)
{
    float eps = .01;
    float3 nx = GetNormal(p + float3(eps, 0, 0)) - GetNormal(p - float3(eps, 0, 0));
    float3 ny = GetNormal(p + float3(0, eps, 0)) - GetNormal(p - float3(0, eps, 0));
    float3 nz = GetNormal(p + float3(0, 0, eps)) - GetNormal(p - float3(0, 0, eps));
    return .5 * (length(nx) + length(ny) + length(nz));
}
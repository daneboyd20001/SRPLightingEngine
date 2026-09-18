struct Light
{
    float3 pos, color;
    float intensity, range;
};

StructuredBuffer<Light> lightBuffer;

#include "SDFs.hlsl"

Texture2D<float3> Col1, Col2, Col3;
SamplerState sampler_Col1, sampler_Col2, sampler_Col3;

static float3 lightSource = float3(sin(Time), cos(Time), 0);
float3 GetSurfaceEmission(float3 p, float3 n)
{
    float3 color1 = float3(1, 0, 0);
    float3 color2 = float3(0, 0, 1);
    float t = (matrixNoise(p) + 1) / 2;
    float v = abs(t - .5) * 2;
    color1 *= (1 - t);
    color2 *= t;
    return (color1 + color2) * v;
}

float3 GetSurfaceColor(float3 p, float3 n)
{
    p /= 2;
    n = n * n;
    n /= n.x + n.y + n.z; //I hate normalizing a normal
    float3 xAxis = Col1.SampleLevel(sampler_Col1, p.yz, 0) * n.x;
    float3 yAxis = Col2.SampleLevel(sampler_Col2, p.zx, 0) * n.y;
    float3 zAxis = Col3.SampleLevel(sampler_Col3, p.xy, 0) * n.z;
    return (xAxis + yAxis + zAxis);
    //return saturate(10 - (length(p)));
}

float3 CalculateDirectHit(float3 pos, float3 dir, float3 normal, float dist)
{
    float3 emission = saturate(GetSurfaceEmission(pos, normal));
    //emission = GetSurfaceEmission(pos, normal);
    
    float c1 = 16;
    float c0 = 20;
    float absorption = exp2(dist / c1);
    float spread = c0 * c0 / ((dist + c0) * (dist + c0));
    
    float rimLight = (1 - dot(dir, -normal));
    rimLight *= rimLight * rimLight;
    float lambertian = dot(dir, -normal);
    float blinnPhong = max(0, dot(normal, normalize(dir + normal)));
    
    //emission *= 2 - absorption;
    emission *= spread;
    return emission * (lambertian);
}

float3 CalculateDiffuseLighting(float3 pos, float3 dir, float3 normal, float dist)
{
    float3 emission = saturate(GetSurfaceEmission(pos, normal));
    //emission = GetSurfaceEmission(pos, normal);
    
    float c1 = 16;
    float c0 = 2;
    float absorption = exp2(-dist / c1);
    float spread = c0 * c0 / ((dist + c0) * (dist + c0));
    
    float rimLight = (1 - dot(dir, -normal));
    rimLight *= rimLight * rimLight;
    float lambertian = dot(dir,-normal);
    float blinnPhong = max(0, dot(normal, normalize(dir + normal)));
    
    //emission *= absorption;
    emission *= spread;
    return emission;
}
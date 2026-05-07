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
    return float3(matrixNoise(p), matrixNoise(p + 3112.2), matrixNoise(p - 123.2));
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


float3 CalculateLighting(float3 pos, float3 dir, float3 normal, float dist)
{
    float3 emission = saturate(GetSurfaceEmission(pos, normal));
    //emission = GetSurfaceEmission(pos, normal);
    
    float c1 = 16;
    float absorption = exp2(-dist / c1);
    float spread = 1 / ((dist + 1) * (dist + 1));
    
    //emission *= absorption;
    emission *= spread;
    return emission;
}
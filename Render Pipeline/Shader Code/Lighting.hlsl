struct Light
{
    float3 pos, color;
    float intensity, range;
};

StructuredBuffer<Light> lightBuffer;

#include "SDFs.hlsl"

Texture2D<float3> Col1, Col2, Col3;
SamplerState sampler_Col1, sampler_Col2, sampler_Col3;


//SMoothramp function 2^4 * x^2 * (x-1)^2
float3 GetSurfaceEmission(float3 p)
{
    //return GetGaussianCurvature(p);
    //return (10 + GetMeanCurvature(p)) / 2;
    //return (matrixNoise(p));
    return matrixNoise(p);
    return (Perlin(p));
    
    
    return SampleColor(p);
    
    float3 xAxis = Col1.SampleLevel(sampler_Col1, p.yz, 0);
    float3 yAxis = Col2.SampleLevel(sampler_Col2, p.zx, 0);
    float3 zAxis = Col3.SampleLevel(sampler_Col3, p.xy, 0);
    return xAxis + yAxis + zAxis / 3;
}

float3 GetSurfaceEmission(float3 p, float3 n)
{
    return float3(matrixNoise(p), matrixNoise(p + 42.12), matrixNoise(96.62));
    n = abs(n);
    n /= n.x + n.y + n.z; //I hate normalizing a normal
    float3 xAxis = Col1.SampleLevel(sampler_Col1, p.yz, 0) * n.x;
    float3 yAxis = Col2.SampleLevel(sampler_Col2, p.zx, 0) * n.y;
    float3 zAxis = Col3.SampleLevel(sampler_Col3, p.xy, 0) * n.z;
    return xAxis + yAxis + zAxis;
}
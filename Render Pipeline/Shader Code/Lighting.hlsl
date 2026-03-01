struct Light
{
    float3 pos, color;
    float intensity, range;
};

StructuredBuffer<Light> lightBuffer;

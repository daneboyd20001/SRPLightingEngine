struct rayHit
{
    float3 normal;
    int pixelID;
    float3 position;
    float depth;
};

RWStructuredBuffer<rayHit> hitBufferRW;
StructuredBuffer<rayHit> hitBuffer;
struct rayHit
{
    float4 posDist;
};

RWStructuredBuffer<rayHit> hitBufferRW;
StructuredBuffer<rayHit> hitBuffer;
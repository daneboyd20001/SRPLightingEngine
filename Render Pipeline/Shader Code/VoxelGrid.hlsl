#define CHUNK_LENGTH 64
#define CHUNK_SPAN 4
#define SCALING_FACTOR 2

RWTexture3D<float4> LOD0, LOD1, LOD2, LOD3, LOD4, LOD5, LOD6, LOD7;
Texture3D<float4> MipMap0, MipMap1, MipMap2, MipMap3, MipMap4, MipMap5, MipMap6, MipMap7;
uint3 lodOffset[8];

SamplerState MipMapSampler;

uint Morton3D(int3 v)
{
    uint3 x = (uint3) v & 0x000003FF;
    x = (x | (x << 16)) & 0x30000FF;
    x = (x | (x << 8)) & 0x300F00F;
    x = (x | (x << 4)) & 0x30C30C3;
    x = (x | (x << 2)) & 0x9249249;
    
    return (x.z << 2) | (x.y << 1) | x.x;
}

uint GetLODID(float3 localPosition)
{
    //divide by half side length
    localPosition /= CHUNK_SPAN;
    localPosition *= 2;
    
    //then calculate depth
    localPosition = ceil(log2(abs(localPosition)));
    return max(max(localPosition.x, localPosition.y), localPosition.z);
}

float3 NormalizePosition(float3 locPos)
{
    //divide by half side length
    locPos /= CHUNK_SPAN;
    locPos *= 2;
    //calculate depth
    float3 logPos = log2(abs(locPos));
    uint maxLog = ceil(max(max(logPos.x, logPos.y), logPos.z));
    uint temp = 1 << maxLog;
    //scale by depth, then normalize
    locPos /= temp;
    return locPos * .5 + .5;
}

//combine into 1 3d texture and modify indexing
//at this point, probably more efficient to create buffers and sample manually, this was only done to take advantage of GPU sampling hardware
float4 SampleVoxelTexture(uint mipID, float3 uvw)
{
    switch (mipID)
    {
        case 0:
            return MipMap0.Sample(MipMapSampler, uvw);
        case 1:
            return MipMap1.Sample(MipMapSampler, uvw);
        case 2:
            return MipMap2.Sample(MipMapSampler, uvw);
        case 3: 
            return MipMap3.Sample(MipMapSampler, uvw);
        case 4:
            return MipMap4.Sample(MipMapSampler, uvw);
        case 5:
            return MipMap5.Sample(MipMapSampler, uvw);
        case 6:
            return MipMap6.Sample(MipMapSampler, uvw);
        default:
            return MipMap7.Sample(MipMapSampler, uvw);
    }
}

float SampleValue(float3 position)
{
    uint MipMapID = GetLODID(position);
    float3 uvw = NormalizePosition(position);
    return SampleVoxelTexture(MipMapID, uvw).w;
}

float3 SampleNormal(float3 position)
{
    uint MipMapID = GetLODID(position);
    float3 uvw = NormalizePosition(position);
    return normalize(SampleVoxelTexture(MipMapID, uvw).xyz);
}

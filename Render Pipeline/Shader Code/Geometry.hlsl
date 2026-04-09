//size  32
struct Vertex
{
    float3 pos;
    float3 normal;
    float2 UV;
};

//size 16
struct Triangle
{
    uint v0;
    uint v1;
    uint v2;
    uint matID;
};

//size 64
struct Material
{
    uint diffuseID;
    uint specularID;
    uint emissiveID;
    uint normalID;
    
    float roughness;
    float metallic;
    float refractiveIndex;
    float opacity;
    
    uint renderFlags;
    float3 anisotropy;
};

//size 32 bits
struct Ray
{
    float3 pos;
    float3 dir;
    float dist;
    uint pixelID;
};

//size 16 bits
struct Hit
{
    float distance;
    float2 bariCoords; //u,v, w = 1 - u - v
    uint hitID;
};

//size 32 bits
struct BVHNode
{
    float2x3 aabbs[64];
};

struct radixNode
{
    float3x2 aabbLeft, aabbRight;
    uint leftChild, rightChild, parent, primativeCount;
};

//primative count only needed for construction, primative index stored implicitely
//parent not needed for traversal if using work shared wavefront method with stack

struct quadNode
{
    float4x3 aabbMins, aabbMaxs;
    uint childStart, childFlag, primativeStart, primativeEnd;
};

uniform StructuredBuffer<Vertex> vertBuffer;
//TODO: Create a index buffer, instead of swapping values at tribuffer, for ownership reasons
uniform RWStructuredBuffer<Triangle> triBuffer;
uniform StructuredBuffer<Material> matBuffer;

uniform RWStructuredBuffer<quadNode> nodeBufferBad;
uniform RWStructuredBuffer<BVHNode> nodeBuffer;
uniform RWStructuredBuffer<uint> keyBuffer;

uniform Texture2DArray texArray;

uniform float4 sceneMin;
uniform float4 sceneMax;
uniform uint triCount;
uniform uint triPower;

bool IntersectTriangle(Ray ray, Triangle tri, out float t, out float2 bary)
{
    float3 edge1 = vertBuffer[tri.v1].pos - vertBuffer[tri.v0].pos;
    float3 edge2 = vertBuffer[tri.v2].pos - vertBuffer[tri.v0].pos;
    float3 h = cross(ray.dir, edge2);
    float a = dot(edge1, h);
    
    t = 1e30;
    bary = float2(0, 0);
    
    //ray is parallel
    if (abs(a) < .001)
    {
        return false;
    }

    float f = 1.0 / a;
    float3 s = ray.pos - vertBuffer[tri.v0].pos;
    float u = f * dot(s, h);
    //if point is inside edge 1 axis
    if (u < 0.0 || u > 1.0)
        return false;

    float3 q = cross(s, edge1);
    float v = f * dot(ray.dir, q);
    //if point is inside edge 2 axis
    if (v < 0.0 || u + v > 1.0)
        return false;

    t = f * dot(edge2, q);
    if (t < 0.0)
        return false; // Triangle is behind ray

    bary = float2(u, v); //w = 1 - u - v
    return true;
}

bool intersectAABB(Ray ray, float3 minAABB, float3 maxAABB)
{
    float3 invDir = 1.0f / ray.dir;

    float3 t0 = (minAABB - ray.pos) * invDir;
    float3 t1 = (maxAABB - ray.pos) * invDir;

    float3 tmin3 = min(t0, t1);
    float3 tmax3 = max(t0, t1);

    float tmin = max(max(tmin3.x, tmin3.y), tmin3.z);
    float tmax = min(min(tmax3.x, tmax3.y), tmax3.z);

    return tmax >= max(tmin, 0.0f);
}

float2 GetUV(Triangle tri, float3 bary)
{
    float2 uv0 = vertBuffer[tri.v0].UV;
    float2 uv1 = vertBuffer[tri.v1].UV;
    float2 uv2 = vertBuffer[tri.v2].UV;
    
    return uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;
}

float3 GetNormal(Triangle tri, float3 bary)
{
    float3 norm0 = vertBuffer[tri.v0].normal;
    float3 norm1 = vertBuffer[tri.v1].normal;
    float3 norm2 = vertBuffer[tri.v2].normal;
    
    return normalize(norm0 * bary.x + norm1 * bary.y + norm2 * bary.z);
}

float2x3 GetAABB(Triangle tri)
{
    float3 p0 = vertBuffer[tri.v0].pos;
    float3 p1 = vertBuffer[tri.v1].pos;
    float3 p2 = vertBuffer[tri.v2].pos;
    
    float3 Min = min(min(p0, p1), p2);
    float3 Max = max(max(p0, p1), p2);
    
    return float2x3(Min, Max);
}

float2x3 GetAABB(float2x3 A, float2x3 B)
{
    float3 Min = min(A[0], B[0]);
    float3 Max = max(A[1], B[1]);
    return float2x3(Min, Max);
}

float3 GetAABBCentroid(float2x3 aabb)
{
    return (aabb[0] + aabb[1]) * .5f;
}

float3 GetTriCentroid(Triangle tri)
{
    return (tri.v0 + tri.v1 + tri.v2) * .33333f;
}

float GetSurfaceArea(float2x3 aabb)
{
    float3 d = aabb[1] - aabb[0];
    d *= 2;
    d = float3(d.x * d.y, d.y * d.z, d.z * d.x);
    return d.x + d.y + d.z;
}
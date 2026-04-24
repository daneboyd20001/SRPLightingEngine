#ifndef SDFs
#define SDFs

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
    return p.z - 1;
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
    float tileSize = 10;
    
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
    
    return (1 - (length(p.xyz / p.w)));
}

float cubeSDF(float4 cube, float3 pos)
{
    float3 d = cube.xyz - pos;
    return max(max(abs(d.x) - cube.w, abs(d.y) - cube.w), abs(d.z) - cube.w);
}

float SDF1(float3 p)
{
    //m = 0; // default material

    float3 mp = fmod(p, 0.1); // periodic tiling
    mp.y = p.y 
           + sin(p.x * 2.0 + Time) * 0.25
           + sin(p.z * 2.5 + Time) * 0.25;

    float PI = 3.14159265;

    // first cube SDF
    float3 pos1 = float3(
        mp.x,
        mp.y + (sin(p.z * PI * 10.0) * sin(p.x * PI * 10.0)) * 0.025,
        0.05
    );
    float s1 = cubeSDF(float4(0.05, 0.05, 0.05, 0.025), pos1);

    // second cube SDF
    float3 pos2 = float3(
        0.05,
        mp.y + (sin(p.x * PI * 10.0) * -sin(p.z * PI * 10.0)) * 0.025,
        mp.z
    );
    float s2 = cubeSDF(float4(0.05, 0.05, 0.05, 0.025), pos2);

    //m = (s1 < s2) ? 0 : 1;

    return min(s1, s2);
}

float SDF2(float3 p0)
{
    p0 /= 11;
    p0.xyz = frac((p0.xyz - 1.0) * 0.5) * 2.0 - 1.0;
    float4 p = float4(p0, 1.0);
    p = abs(p);

    if (p.x < p.z)
        p.xz = p.zx;
    if (p.z < p.y)
        p.zy = p.yz;
    if (p.y < p.x)
        p.yx = p.xy;
    
    for (int i = 0; i < 10; i++)
    {
        if (p.x < p.z)
            p.xz = p.zx;
        if (p.z < p.y)
            p.zy = p.yz;
        if (p.y < p.x)
            p.yx = p.xy;

        p.xyz = abs(p.xyz);
        float dotVal = dot(p.xyz, p.xyz);
        p.xyz *= 1.6 / clamp(dotVal, .1, 1);
        p.xyz -= float3(0.7, 1.8, 0.5);
        p.xyz *= 1.2;
    }

    float m = 1.5;
    p.xyz -= clamp(p.xyz, -m, m);

    return (length(p.xyz) / p.w) / 200.0;
}

float SDF3(float3 p)
{
    const float TAUg = atan(1.0) * 8.0;

    for (int i = 0; i < 4; i++)
    {
        // p.xy = pmodg(p.xy, 10.)
        float ang_xy = atan2(p.y, p.x);
        float seg_xy = TAUg / 10.0;
        float a_xy = (ang_xy - seg_xy * floor(ang_xy / seg_xy)) - 0.5 * seg_xy;
        float r_xy = length(p.xy);
        p.xy = r_xy * float2(sin(a_xy), cos(a_xy));

        p.y -= 2.0;

        // p.yz = pmodg(p.yz, 12.)
        float ang_yz = atan2(p.z, p.y);
        float seg_yz = TAUg / 12.0;
        float a_yz = (ang_yz - seg_yz * floor(ang_yz / seg_yz)) - 0.5 * seg_yz;
        float r_yz = length(p.yz);
        p.yz = r_yz * float2(sin(a_yz), cos(a_yz));

        p.z -= 10.0;
    }

    float3 n = normalize(float3(13.0, 1.0, 7.0));
    return dot(abs(p), n) - 0.7;
}

float SDF4(float3 p0)
{
    p0 /= 6;
    float4 p = float4(p0, 1);
    float3 cVal = float3(1., 1., 1.);
    float scale = 1;
    for (int i = 0; i < 16; i++)
    {
        p.xyz = 2.0 * clamp(p.xyz, -cVal, cVal) - p.xyz;
        p *= max(.1, 2 / dot(p.xyz, p.xyz));
        // Scale and offset by original point
        p.xyz = p.xyz * scale;
        p.w *= abs(scale);
    }
    
    return ((length(p.xyz / p.w)));
}

float WierdTriangleSDF(float3 p0)
{
    float4 p = float4(p0, 1.0);
    //p = sqrt(p);
    for (int i = 0; i < 14; i++)
    {
        p.xyz = frac((p.xyz - 1.0) * 0.5) * 2.0 - 1.0;
        p *= 1.4 / dot(p.xyz, p.xyz);
    }

    return length(p.xyz / p.w);
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

float SDF5(float3 p)
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
    p = p.xzy;
    float3 cSize = float3(1., 1., 1.3);
    float scale = 1.;
    for (int i = 0; i < 12; i++)
    {
        p = 2.0 * clamp(p, -cSize, cSize) - p;
        float r2 = dot(p, p + sin(p.z * .3));
        float k = max((2.) / (r2), .027);
        p *= k;
        scale *= k;
    }
    float l = length(p.xy);
    float rxy = l - 4.0;
    float n = l * p.z;
    rxy = max(rxy, -(n) / 4.);
    return (rxy) / abs(scale);
}
float NoiseSDF(float3 pos)
{
    float dist = 0;
    
    float s = 2;
    float3 hashx = float3(971.23, 231.67, 753.91);
    float3 hashy = float3(421.38, 882.19, 1193.57);
    for (int i = 0; i < 3; i++)
    {
        dist += matrixNoise(pos / s + hashx) * s;
        hashx += hashy;
        s /= 2;
    }
    
    return (dist) * 2;
}

float expSDF(float3 pos)
{
    float r = length(pos);
    float theta = atan2(pos.y , pos.x);
    float phi = acos(pos.z / r);
    
    //Spherical Encoding
    float3 p = float3(log2(r), theta, phi);
    float e = theta - 1.5;
    
    for (int i = 1; i < 256; i = i << 1)
    {
        e += sqrt(abs(dot(sin(p.xxx * i), cos(p * i)))) / i;
    }
    
    return (Time + e * r);

}

float SDF(float3 p)
{
    return expSDF(p);
}


float3 GetGradient(float3 p)
{
    float eps = .02;
    
    float3 ex = float3(eps, 0, 0);
    float3 ey = float3(0, eps, 0);
    float3 ez = float3(0, 0, eps);
    
    float3 diff = float3(
        SDF(p + ex) - SDF(p - ex),
        SDF(p + ey) - SDF(p - ey),
        SDF(p + ez) - SDF(p - ez)
    );
    
    return diff / (eps * 2);
}

float3 GetNormal(float3 p)
{
    float eps = .02;
    
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

//https://backend.orbit.dtu.dk/ws/files/126824972/onb_frisvad_jgt2012_v2.pdf#:~:text=This%20is%20useful%20in%20Monte%20Carlo%20rendering,in%20spherical%20coordinates%20and%20then%20transformed%20to
float3 GetTangent(float3 p, float theta)
{
    float3 normal = GetNormal(p);
    float3 b1, b2;
    if (normal.z < -.99999999)
    {
        b1 = float3(0, -1, 0);
        b2 = float3(-1, 0, 0);
    }
    else
    {
        float a = 1.0f / (1.0f + normal.z);
        float b = -a * normal.x * normal.y;
        b1 = float3(1.0f - normal.x * normal.x * a, b, -normal.x);
        b2 = float3(b, 1.0f - normal.y * normal.y * a, -normal.y);
    }
    return b1 * sin(theta) + b2 * cos(theta);
}

float3x3 GetHessian(float3 p)
{
    float eps = .02;
    float3 ddx = GetGradient(p + float3(eps, 0, 0)) - GetGradient(p - float3(eps, 0, 0)) / (2 * eps);
    float3 ddy = GetGradient(p + float3(0, eps, 0)) - GetGradient(p - float3(0, eps, 0)) / (2 * eps);
    float3 ddz = GetGradient(p + float3(0, 0, eps)) - GetGradient(p - float3(0, 0, eps)) / (2 * eps);
    
    float3x3 H;
    H[0] = float3(ddx.x, ddy.x, ddz.x);
    H[1] = float3(ddx.y, ddy.y, ddz.y);
    H[2] = float3(ddx.z, ddy.z, ddz.z);
    return H;
}

float GetLaplacian(float3 p)
{
    float eps = .02;
    float diff = SDF(p + float3(eps, 0, 0)) + SDF(p - float3(eps, 0, 0)) +
                 SDF(p + float3(0, eps, 0)) + SDF(p - float3(0, eps, 0)) +
                 SDF(p + float3(0, 0, eps)) + SDF(p - float3(0, 0, eps));
    return (diff - SDF(p) * 6) / (eps * eps);
}

//Divergence of normal
float GetMeanCurvature(float3 p)
{
    return -GetLaplacian(p) / (2 * length(GetGradient(p)));
}
float GetGaussianCurvature(float3 p)
{
    float3x3 hess = GetHessian(p);
    float3 grad = GetGradient(p);
    float4x4 mat = float4x4(hess[0][0], hess[0][1], hess[0][2], grad.x,
                            hess[1][0], hess[1][1], hess[1][2], grad.y,
                            hess[2][0], hess[2][1], hess[2][2], grad.z,
                            grad.x, grad.y, grad.z, 0);
    //Formula needs len^4
    float len = dot(grad, grad);
    len *= len;
    return -determinant(mat) / len;
}
#endif
using PathtracingPipeline;
using System.Collections.Generic;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.Rendering;

public class CustomRenderPipeline : RenderPipeline
{
    public CustomRenderPipelineAsset asset;
    public RenderTexture rayResult;

    public ComputeBuffer vertBuffer, triBuffer, matBuffer, keyBuffer, nodeBuffer, rayBuffer, hitBuffer;
    public RenderTexture NoiseTexture;

    public List<Vertex> vertList;
    public List<Triangle> triList;

    int vertSize = 1 << 18, triSize = 1 << 17, matSize = 1 << 10;

    float camMinClip, camMaxClip, FOV;
    public void Init()
    {
        rayResult = new RenderTexture(Screen.width, Screen.height, 0);
        rayResult.enableRandomWrite = true;
        rayResult.Create();

        hitBuffer = new ComputeBuffer(Screen.width * Screen.height, 16);
    }

    protected override void Render(ScriptableRenderContext context, List<Camera> cameras)
    {
        CommandBuffer cmd = CommandBufferPool.Get("SceneData");

        //SetGeometryData(cmd);
        //ConstructBVH(cmd);
        //context.ExecuteCommandBuffer(cmd);
        CommandBufferPool.Release(cmd);

        cmd = CommandBufferPool.Get("Debug");
        //DebugStuff2(cmd);
        //context.ExecuteCommandBuffer(cmd);
        CommandBufferPool.Release(cmd);

        for (int i = 0; i < cameras.Count; i++)
        {
            cmd = CommandBufferPool.Get("CameraData");
            cmd.SetGlobalBuffer("hitBuffer", hitBuffer);
            cmd.SetGlobalBuffer("hitBufferRW", hitBuffer);
            SetCamData(cmd, cameras[i]);
            RenderSDFRaymarch(cmd);
            //RenderPathTrace(cmd);
            context.ExecuteCommandBuffer(cmd);
            CommandBufferPool.Release(cmd);

            context.DrawUIOverlay(cameras[i]);
            context.Submit();
        }
    }

    void SetCamData(CommandBuffer cmd, Camera cam)
    {
        FOV = cam.fieldOfView;
        camMinClip = cam.nearClipPlane;
        camMaxClip = cam.farClipPlane;

        cmd.SetGlobalMatrix("WorldToCam", cam.worldToCameraMatrix);
        cmd.SetGlobalMatrix("CamToWorld", cam.cameraToWorldMatrix);
        cmd.SetGlobalMatrix("ClipMatrix", cam.projectionMatrix);

        cmd.SetGlobalVector("ScreenSize", new Vector4(Screen.width, Screen.height, (float)Screen.width / (float)Screen.height, (float)Screen.height / (float)Screen.width));
        cmd.SetGlobalFloat("Time", Time.time);
        cmd.SetGlobalFloat("FOV_Tan", Mathf.Tan(FOV * .5f * 3.14159f / 180f));
        cmd.SetGlobalFloat("MinClip", camMinClip);
        cmd.SetGlobalFloat("MaxClip", camMaxClip);

        cmd.ClearRenderTarget(true, true, Color.black);
    }
    void SetGeometryData(CommandBuffer cmd)
    {
        if (vertBuffer == null || Time.frameCount % 60 == 0)
        {
            DisposeGeometryData();
            Geometry.ConstructSceneData();

            //its helpful to have data a power of 2
            vertSize = Geometry.vertexData.Count;
            triSize = Geometry.triangleData.Count;

            vertSize = math.max(triSize, 256);
            triSize = math.max(triSize, 256);

            vertBuffer = new ComputeBuffer(vertSize, 32);
            triBuffer = new ComputeBuffer(triSize, 16);
            matBuffer = new ComputeBuffer(matSize, 48);

            //used in bvh construction
            keyBuffer = new ComputeBuffer(triSize, 4);
            //Adding 1 index for parent, this allows easier traversal, and construction of AABBs probably not ideal
            nodeBuffer = new ComputeBuffer((triSize), 36);

            rayBuffer = new ComputeBuffer(1920 * 1080, 32);

            vertBuffer.SetData(Geometry.vertexData, 0, 0, Geometry.vertexData.Count);
            triBuffer.SetData(Geometry.triangleData, 0, 0, Geometry.triangleData.Count);
        }
        cmd.SetGlobalBuffer("vertBuffer", vertBuffer);
        cmd.SetGlobalBuffer("triBuffer", triBuffer);
        cmd.SetGlobalBuffer("matBuffer", matBuffer);

        //used to create and store BVH
        cmd.SetGlobalBuffer("keyBuffer", keyBuffer);
        cmd.SetGlobalBuffer("nodeBuffer", nodeBuffer);

        //used to store ray Data
        cmd.SetGlobalBuffer("rayBuffer", rayBuffer);

        cmd.SetGlobalInteger("triCount", Geometry.triangleData.Count);
        cmd.SetGlobalInteger("triPower", math.ceillog2(triSize));
        cmd.SetGlobalVector("sceneMin", new float4(Geometry.sceneMin, 0));
        cmd.SetGlobalVector("sceneMax", new float4(Geometry.sceneMax, 0));
    }
    void ConstructBVH(CommandBuffer cmd)
    {
        int Init = asset.BuildBVH.FindKernel("CSInit");
        cmd.DispatchCompute(asset.BuildBVH, Init, (1 << triSize) / 64, 1, 1);

        int bitonicSort = asset.BuildBVH.FindKernel("CSBitonicSort");

        for (int i = 0; i < triSize - 1; i++)
        {
            for (int j = i; j >= 0; --j)
            {
                cmd.SetComputeIntParam(asset.BuildBVH, "depth", i);
                cmd.SetComputeIntParam(asset.BuildBVH, "step", j);
                cmd.DispatchCompute(asset.BuildBVH, bitonicSort, (1 << triSize) >> 7, 1, 1);
            }
        }

        int build = asset.BuildBVH.FindKernel("CSBuild");
        for (int i = triSize; i >= 6; i--)
        {
            int dispatchCount = 1 << i;
            dispatchCount /= 64;
            dispatchCount = math.max(1, dispatchCount);
            cmd.SetComputeIntParam(asset.BuildBVH, "depth", i);
            cmd.DispatchCompute(asset.BuildBVH, build, dispatchCount, 1, 1);
        }

        build = asset.BuildBVH.FindKernel("CSBuildFinal");
        cmd.DispatchCompute(asset.BuildBVH, build, 1, 1, 1);
    }

    void RenderSDFRaymarch(CommandBuffer cmd)
    {
        int kernel = asset.RayMarch.FindKernel("CSPrimaryRay");
        cmd.DispatchCompute(asset.RayMarch, kernel, Screen.width / 8, Screen.height / 8, 1);

        kernel = asset.RayMarch.FindKernel("CSDirectHit");
        cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Result", rayResult);
        cmd.DispatchCompute(asset.RayMarch, kernel, Screen.width / 8, Screen.height / 8, 1);
        cmd.Blit(rayResult, BuiltinRenderTextureType.CameraTarget, new Vector2(1f, 1f), Vector2.zero);
    }

    void RenderPathTrace(CommandBuffer cmd)
    {
        //primary rays
        int kernal = asset.PathTrace.FindKernel("CameraRayGeneration");
        cmd.SetComputeTextureParam(asset.PathTrace, kernal, "Result", rayResult);
        cmd.DispatchCompute(asset.PathTrace, kernal, Screen.width / 8, Screen.height / 8, 1);

        cmd.Blit(rayResult, BuiltinRenderTextureType.CameraTarget);
    }

    void DebugStuff2(CommandBuffer cmd)
    {
        cmd.SetGlobalBuffer("prefixSum", keyBuffer);

        int kernal = asset.BuildBVH2.FindKernel("CSInit");
        cmd.DispatchCompute(asset.BuildBVH2, kernal, triSize / 64, 1, 1);
        DebugStuff(cmd);

        kernal = asset.BuildBVH2.FindKernel("CSBinPrefix");
        cmd.DispatchCompute(asset.BuildBVH2, kernal, triSize / 64, 1, 1);
        DebugStuff(cmd);

        kernal = asset.BuildBVH2.FindKernel("CSUpSweep");
        int depth = 0;
        for (int i = 1; i < triSize; i = i << 1)
        {
            cmd.SetComputeIntParam(asset.BuildBVH2, "depth", depth);
            /*
             * for d = 0 to log2(n) – 1 do 
             * for all k = 0 to n – 1 by 2^(d+1) in parallel do 
             * x[k +  2^(d+1) – 1] = x[k +  2^d  – 1] + x[k +  2^(d+1) – 1]
            */
            cmd.DispatchCompute(asset.BuildBVH2, kernal, math.max(1, (triSize * 64 / 256) >> (depth + 1)), 1, 1);
            depth++;
            DebugStuff(cmd);
        }

    }

    void DebugStuff(CommandBuffer cmd)
    {
        int kernal = asset.Debug.FindKernel("DebugSorting");
        cmd.SetComputeTextureParam(asset.Debug, kernal, "Result", rayResult);
        cmd.DispatchCompute(asset.Debug, kernal, Screen.width / 8, Screen.height / 8, 1);
        cmd.Blit(rayResult, BuiltinRenderTextureType.CameraTarget);
    }

    protected override void Dispose(bool disposing)
    {
        Debug.Log("Disposing render pipeline");
        base.Dispose(disposing);
        rayResult.DiscardContents();

        DisposeGeometryData();
    }

    void DisposeGeometryData()
    {
        if (vertBuffer != null)
        {
            vertBuffer.Dispose();
            triBuffer.Dispose();
            matBuffer.Dispose();

            keyBuffer.Dispose();
            nodeBuffer.Dispose();

            rayBuffer.Dispose();
            //hitBuffer.Dispose();
        }
    }
}
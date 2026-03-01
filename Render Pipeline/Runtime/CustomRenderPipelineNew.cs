using PathtracingPipeline;
using System.Collections.Generic;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.Rendering;

public class CustomRenderPipelineNew : RenderPipeline
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

        CommandBufferPool.Release(cmd);

        cmd = CommandBufferPool.Get("Debug");

        CommandBufferPool.Release(cmd);

        for (int i = 0; i < cameras.Count; i++)
        {
            cmd = CommandBufferPool.Get("CameraData");
            cmd.SetGlobalBuffer("hitBuffer", hitBuffer);
            cmd.SetGlobalBuffer("hitBufferRW", hitBuffer);
            SetCamData(cmd, cameras[i]);



            context.ExecuteCommandBuffer(cmd);
            CommandBufferPool.Release(cmd);
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
    void RenderSDFRaymarch(CommandBuffer cmd)
    {
        int kernel = asset.RayMarch.FindKernel("CSPrimaryRay");
        cmd.DispatchCompute(asset.RayMarch, kernel, Screen.width / 8, Screen.height / 8, 1);

        kernel = asset.RayMarch.FindKernel("CSDirectHit");
        cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Result", rayResult);
        cmd.DispatchCompute(asset.RayMarch, kernel, Screen.width / 8, Screen.height / 8, 1);
        cmd.Blit(rayResult, BuiltinRenderTextureType.CameraTarget, new Vector2(1f,1f), Vector2.zero);
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
        if (hitBuffer != null)
        {
            hitBuffer.Dispose();
        }
    }
}
using PathtracingPipeline;
using System.Collections.Generic;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.Rendering;

public class CustomRenderPipeline : RenderPipeline
{
    public CustomRenderPipelineAsset asset;
    public RenderSettings settings;
    public RenderTexture rayResult;

    public ComputeBuffer hitBuffer;
    public RenderTexture NoiseTexture;

    float camMinClip, camMaxClip, FOV;
    public void Init()
    {
        rayResult = new RenderTexture(Screen.width, Screen.height, 0);
        rayResult.enableRandomWrite = true;
        rayResult.Create();

        hitBuffer = new ComputeBuffer(Screen.width * Screen.height, 16);

        NoiseTexture = new RenderTexture(4096, 4096, 0);
        NoiseTexture.wrapMode = TextureWrapMode.Repeat;
        NoiseTexture.enableRandomWrite = true;
        NoiseTexture.Create();

        CommandBuffer cmd = CommandBufferPool.Get("Initialization");
        int kernel = asset.GenerateNoise.FindKernel("CSMain");
        cmd.SetComputeTextureParam(asset.GenerateNoise, kernel, "Result", NoiseTexture);
        cmd.DispatchCompute(asset.GenerateNoise, kernel, 512, 512, 1);
        Graphics.ExecuteCommandBuffer(cmd);
    }

    protected override void Render(ScriptableRenderContext context, List<Camera> cameras)
    {
        CommandBuffer cmd = CommandBufferPool.Get("SceneData");
        CommandBufferPool.Release(cmd);

        for (int i = 0; i < cameras.Count; i++)
        {
            cmd = CommandBufferPool.Get("CameraData");
            cmd.SetGlobalBuffer("hitBuffer", hitBuffer);
            cmd.SetGlobalBuffer("hitBufferRW", hitBuffer);
            cmd.SetGlobalTexture("NoiseTex", NoiseTexture);
            SetCamData(cmd, cameras[i]);

            RenderSDFRaymarch(cmd);

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
    void RenderSDFRaymarch(CommandBuffer cmd)
    {
        int kernel = asset.RayMarch.FindKernel("CSPrimaryRay");
        cmd.SetComputeFloatParam(asset.RayMarch,"quality", 1 / settings.RaymarchQuality);
        cmd.SetComputeFloatParam(asset.RayMarch, "maxSteps", settings.MaximumStepCount);
        cmd.DispatchCompute(asset.RayMarch, kernel, Screen.width / 8, Screen.height / 8, 1);

        kernel = asset.RayMarch.FindKernel("CSDirectHit");
        cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Col1", settings.xAxisColor);
        cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Col2", settings.yAxisColor);
        cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Col3", settings.zAxisColor);
        cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Result", rayResult);
        cmd.DispatchCompute(asset.RayMarch, kernel, Screen.width / 8, Screen.height / 8, 1);

        if (settings.UseAmbientOcclusion)
        {
            kernel = asset.RayMarch.FindKernel("CSAmbientOcclusion");
            cmd.SetComputeTextureParam(asset.RayMarch, kernel, "Result", rayResult);
            uint x, y, z;
            asset.RayMarch.GetKernelThreadGroupSizes(kernel, out x, out y, out z);
            cmd.DispatchCompute(asset.RayMarch, kernel, (int)(Screen.width / y), (int)(Screen.height / y), 1);
        }

        cmd.Blit(rayResult, BuiltinRenderTextureType.CameraTarget, new Vector2(1f, 1f), Vector2.zero);
    }

    protected override void Dispose(bool disposing)
    {
        Debug.Log("Disposing render pipeline");
        base.Dispose(disposing);
        rayResult.DiscardContents();
        hitBuffer.Dispose();
    }
}
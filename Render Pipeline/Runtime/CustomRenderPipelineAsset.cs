using System;
using UnityEngine;
using UnityEngine.Rendering;

public class CustomRenderPipelineAsset : RenderPipelineAsset<CustomRenderPipeline>
{   
    [Header("SDF Rendering")]
    public ComputeShader RayMarch;
    [Header("Noise Rendering")]
    public ComputeShader GenerateNoise;

    [Header("Rendering Settings")]
    public float SDFScale;
    public float minDist;
    public int maxStepCount;

    [Header("Lighting Settings")]
    public bool useAmbientOcclusion;

    protected override RenderPipeline CreatePipeline()
    {
        // prints maximum graphics buffer size, in megabytes
        var maxSizeMb = SystemInfo.maxGraphicsBufferSize / 1024 / 1024;
        UnityEngine.Debug.Log("Max size of a compute buffer in MB: " + maxSizeMb);

        var pipeline = new CustomRenderPipeline();
        pipeline.asset = this;
        pipeline.Init();
        return pipeline;
    }

}

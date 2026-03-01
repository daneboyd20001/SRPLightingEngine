using System;
using UnityEngine;
using UnityEngine.Rendering;

public class CustomRenderPipelineAsset : RenderPipelineAsset<CustomRenderPipeline>
{   
    [Header("Geometry Rendering")]
    public ComputeShader PathTrace;
    [Header("SDF Rendering")]
    public ComputeShader RayMarch;
    public ComputeShader BuildBVH, BuildBVH2;
    [Header("Voxel Based Rendering")]
    public ComputeShader VoxelMarch, ConstructVoxel;
    public ComputeShader Debug;
    [Header("Noise Rendering")]
    public ComputeShader GenerateNoise;

    public Vector3 lightPosition;
    public float lightRadius;
    public float lightIntensity;
    public Color lightColor; 

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

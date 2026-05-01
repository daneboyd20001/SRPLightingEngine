using System.Collections.Generic;
using Unity;
using UnityEngine;

[CreateAssetMenu]
public class RenderSettings : ScriptableObject
{
    [Header("Performance Settings")]
    public float RaymarchQuality;
    public int MaximumStepCount;

    [Header("Render Features")]
    public bool UseAmbientOcclusion;
    public bool UseDiffuseBounces;

    [Header("Texture")]
    public Texture xAxisColor, yAxisColor, zAxisColor;
}

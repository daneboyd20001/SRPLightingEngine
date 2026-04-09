using System.Collections.Generic;
using Unity;
using UnityEngine;

[CreateAssetMenu]
public class RenderSettings : ScriptableObject
{
    public enum RenderType
    {
        DanesSDF, SmoothNoise, FractalNoise
    }

    public List<RenderPass> OrderedPasses;
}

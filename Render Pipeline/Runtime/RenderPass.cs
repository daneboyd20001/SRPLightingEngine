using UnityEngine;
using UnityEngine.Rendering;

public abstract class RenderPass : ScriptableObject
{
    public abstract void Initialize();
    public abstract void Render(CustomRenderPipeline pipeline);
    public abstract void Destroy();

}

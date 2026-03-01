using Unity.VisualScripting;
using UnityEngine;

[CreateAssetMenu]
public class SDFTexture : ScriptableObject
{
    //public Texture2D texture;
    public ComputeShader SDFShader;
    public bool regenerate;
    public bool rebindShader;

    public Vector3 CamPos = new Vector3(1, 1, 1.3f);
    public Vector3 CamForward = new Vector3(1, 0, 0);
    public float scale = 1f;
    public float distortion = 1;
    public int iterations = 12;


    RenderTexture texture;
    public Texture2D texture2D;


    private void OnValidate()
    {
        Render();
    }

    void Render()
    {
        if (!regenerate) return;

        if (rebindShader)
        {
            texture = new RenderTexture(2048, 2048, 1);
            texture.enableRandomWrite = true;
            texture.Create();

            if (SDFShader == null || texture2D == null)
            {
                Debug.Log("Either Shader or texture is Null!");
                return;
            }

            SDFShader.SetTexture(0, "Result", texture);
        }
        SDFShader.SetFloats("CamPos", new float[] { CamPos.x, CamPos.y, CamPos.z });
        SDFShader.SetFloats("CamForward", new float[] {CamForward.x, CamForward.y, CamForward.z});
        SDFShader.SetInt("maxSteps", iterations);
        SDFShader.SetFloat("scale", scale); 
        SDFShader.Dispatch(SDFShader.FindKernel("CSMain"), 2048 / 8, 2048 / 8, 1);
        Graphics.CopyTexture(texture, texture2D);
    }
}
/*
 * float de( vec3 p ){
    p = p.xzy;
    vec3 cSize = vec3(1., 1., 1.3);
    float scale = 1.;
    for( int i=0; i < 12; i++ ){
      p = 2.0*clamp(p, -cSize, cSize) - p;
      float r2 = dot(p,p+sin(p.z*.3));
      float k = max((2.)/(r2), .027);
      p *= k;  scale *= k;
    }
    float l = length(p.xy);
    float rxy = l - 4.0;
    float n = l * p.z;
    rxy = max(rxy, -(n) / 4.);
    return (rxy) / abs(scale);
  }
 */
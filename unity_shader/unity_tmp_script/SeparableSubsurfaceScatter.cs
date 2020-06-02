using System.Collections;
using System.Collections.Generic;
using UnityEngine.Rendering;
using UnityEngine;
using UnityEditor;
using UnityEngine.UI;

[ExecuteInEditMode]
[RequireComponent(typeof(Camera))]
public class SeparableSubsurfaceScatter : MonoBehaviour {
	///////SSS Property
	[Range(0,6)]
	public float SubsurfaceScaler = 0.25f;
    public Color SubsurfaceColor;
    public Color SubsurfaceFalloff;
	private Camera RenderCamera;
	private CommandBuffer SubsurfaceBuffer;
    private CommandBuffer SpecularBuffer;
	private Material SubsurfaceEffects = null;
    private Material SpecularEffects = null;
    private List<Vector4> KernelArray = new List<Vector4>();
	static int SceneColorID = Shader.PropertyToID("_SceneColor");
    static int SpecularID = Shader.PropertyToID("_Specular");
    static int Kernel = Shader.PropertyToID("_Kernel");
    static int SSSScaler = Shader.PropertyToID("_SSSScale");
    private RenderTexture spec_rt;

    public GameObject head;

	void OnEnable() {
        ///Initialization SubsurfaceMaskBufferProperty
        RenderCamera = GetComponent<Camera>();
        SpecularBuffer = new CommandBuffer();
        SpecularEffects = new Material(Shader.Find("my_shader/skin_specular"));
        SpecularBuffer.name = "4S skin specular";
        RenderCamera.AddCommandBuffer(CameraEvent.AfterForwardOpaque, SpecularBuffer);


        SubsurfaceBuffer = new CommandBuffer();
		SubsurfaceEffects = new Material(Shader.Find("Hidden/SeparableSubsurfaceScatter"));
        SubsurfaceBuffer.name = "Separable Subsurface Scatter";
        RenderCamera.clearStencilAfterLightingPass = true;  //Clear Deferred Stencil
		RenderCamera.AddCommandBuffer(CameraEvent.AfterForwardOpaque, SubsurfaceBuffer);

        spec_rt = new RenderTexture(RenderCamera.pixelWidth, RenderCamera.pixelHeight, 0);

        show_rt(spec_rt);
    }

    void OnDisable() {
        ClearSubsurfaceBuffer();
        ClearSpecularBuffer();
    }

	void OnPreRender() {
		UpdateSubsurface();
	}

    void show_rt(RenderTexture rt)
    {
        RawImage img = GameObject.Find("Canvas/show_rt").GetComponent<RawImage>();
        img.texture = rt;
    }

	void UpdateSubsurface() {
        ///渲染specular color
        Mesh mesh = head.GetComponent<MeshFilter>().mesh;
        SpecularBuffer.Clear();
        SpecularBuffer.SetRenderTarget(spec_rt);
        //SpecularBuffer.GetTemporaryRT(SpecularID, RenderCamera.pixelWidth, RenderCamera.pixelHeight, 0, FilterMode.Trilinear, RenderTextureFormat.DefaultHDR);
        SpecularBuffer.DrawMesh(mesh, head.transform.localToWorldMatrix, SpecularEffects, 0, 0);

        ///SSS Color
        Vector3 SSSC = Vector3.Normalize(new Vector3 (SubsurfaceColor.r, SubsurfaceColor.g, SubsurfaceColor.b));
		Vector3 SSSFC = Vector3.Normalize(new Vector3 (SubsurfaceFalloff.r, SubsurfaceFalloff.g, SubsurfaceFalloff.b));
		SeparableSSS.CalculateKernel(KernelArray, 25, SSSC, SSSFC);
		SubsurfaceEffects.SetVectorArray (Kernel, KernelArray);
		SubsurfaceEffects.SetFloat (SSSScaler, SubsurfaceScaler);
		///SSS Buffer
		SubsurfaceBuffer.Clear();
		SubsurfaceBuffer.GetTemporaryRT (SceneColorID, RenderCamera.pixelWidth, RenderCamera.pixelHeight, 0, FilterMode.Trilinear, RenderTextureFormat.DefaultHDR);

        SubsurfaceBuffer.BlitStencil(BuiltinRenderTextureType.CameraTarget, SceneColorID, BuiltinRenderTextureType.CameraTarget, SubsurfaceEffects, 0);
        SubsurfaceBuffer.BlitSRT(SceneColorID, BuiltinRenderTextureType.CameraTarget, SubsurfaceEffects, 1);
    }

	void ClearSubsurfaceBuffer() {
		SubsurfaceBuffer.ReleaseTemporaryRT(SceneColorID);
		RenderCamera.RemoveCommandBuffer(CameraEvent.BeforeImageEffectsOpaque, SubsurfaceBuffer);
		SubsurfaceBuffer.Release();
		SubsurfaceBuffer.Dispose();
	}

    void ClearSpecularBuffer()
    {
        //SpecularBuffer.ReleaseTemporaryRT(SpecularID);
        RenderCamera.RemoveCommandBuffer(CameraEvent.BeforeImageEffectsOpaque, SpecularBuffer);
        SpecularBuffer.Release();
        SpecularBuffer.Dispose();
    }


    void SetFPSFrame(bool UseHighFPS, int TargetFPS) {
		if(UseHighFPS == true){
			QualitySettings.vSyncCount = 0;
			Application.targetFrameRate = TargetFPS;
		}
		else{
			QualitySettings.vSyncCount = 1;
			Application.targetFrameRate = 60;
		}
	}
}

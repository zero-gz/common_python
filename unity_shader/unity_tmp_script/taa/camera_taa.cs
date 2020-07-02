using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[ExecuteInEditMode]
public class camera_taa : MonoBehaviour {
    private Camera m_cam;
    private Material m_mat;
    private RenderTexture pre_tex;
    private FrustumJitter _test_frust_jitter;
    private VelocityBuffer _velocityBuffer;

    [Range(0.0f, 1.0f)] public float feedbackMin = 0.88f;
    [Range(0.0f, 1.0f)] public float feedbackMax = 0.97f;

    // Use this for initialization
    void Start () {
		
	}

    private void OnEnable()
    {
        m_cam = GetComponent<Camera>();
        m_cam.depthTextureMode = DepthTextureMode.Depth | DepthTextureMode.MotionVectors;
        _test_frust_jitter = GetComponent<FrustumJitter>();
        _velocityBuffer = GetComponent<VelocityBuffer>();

        m_mat = new Material(Shader.Find("my_shader/test_taa"));
        pre_tex = new RenderTexture(Screen.width, Screen.height, 0);
        pre_tex.name = "pre_texture";

        Shader.SetGlobalTexture(Shader.PropertyToID("_PrevTex"), pre_tex);
    }

    // Update is called once per frame
    void Update () {
		
	}

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        Vector4 jitterUV = _test_frust_jitter.activeSample;
        jitterUV.x /= source.width;
        jitterUV.y /= source.height;
        jitterUV.z /= source.width;
        jitterUV.w /= source.height;

        m_mat.SetVector("_JitterUV", jitterUV);
        m_mat.SetTexture("_VelocityBuffer", _velocityBuffer.activeVelocityBuffer);
        m_mat.SetFloat("_FeedbackMin", feedbackMin);
        m_mat.SetFloat("_FeedbackMax", feedbackMax);
        m_mat.SetTexture("_PrevTex", pre_tex);

        RenderTexture internalDestination = RenderTexture.GetTemporary(source.width, source.height, 0, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Default, source.antiAliasing);
        // 这里为什么不能直接拷贝到backbuffer上？？
        Graphics.Blit(source, internalDestination, m_mat, 0);
        Graphics.Blit(internalDestination, destination);
        Graphics.Blit(internalDestination, pre_tex);
        RenderTexture.ReleaseTemporary(internalDestination);
    }
}

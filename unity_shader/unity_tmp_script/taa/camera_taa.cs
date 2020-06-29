using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[ExecuteInEditMode]
public class camera_taa : MonoBehaviour {
    private Camera m_cam;
    private Material m_mat;
    private RenderTexture pre_tex;

    [Range(0.0f, 1.0f)] public float feedbackMin = 0.88f;
    [Range(0.0f, 1.0f)] public float feedbackMax = 0.97f;

    // Use this for initialization
    void Start () {
		
	}

    private void OnEnable()
    {
        m_cam = GetComponent<Camera>();
        m_cam.depthTextureMode = DepthTextureMode.Depth | DepthTextureMode.MotionVectors;

        m_mat = new Material(Shader.Find("my_shader/custom_taa"));
        pre_tex = new RenderTexture(Screen.width, Screen.height, 0);

        Shader.SetGlobalTexture(Shader.PropertyToID("_PrevTex"), pre_tex);
    }

    // Update is called once per frame
    void Update () {
		
	}

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        m_mat.SetFloat("_FeedbackMin", feedbackMin);
        m_mat.SetFloat("_FeedbackMax", feedbackMax);

        Graphics.Blit(source, destination, m_mat);
        Graphics.Blit(destination, pre_tex);
    }
}

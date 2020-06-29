using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class camera_taa : MonoBehaviour {
    private Camera m_cam;
    private Material m_mat;

    // Use this for initialization
    void Start () {
		
	}

    private void OnEnable()
    {
        m_cam = GetComponent<Camera>();
        m_cam.depthTextureMode = DepthTextureMode.Depth | DepthTextureMode.MotionVectors;

        m_mat = new Material(Shader.Find("my_shader/custom_taa"));
    }

    // Update is called once per frame
    void Update () {
		
	}

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        Graphics.Blit(source, destination, m_mat);
    }
}

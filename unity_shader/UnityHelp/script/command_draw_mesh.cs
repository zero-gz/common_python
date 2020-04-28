using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.UI;

public class command_draw_mesh : MonoBehaviour {
    private RenderTexture rt;
    private Camera m_Camera;
    //这里应该是按拍图对应mesh的长和宽来
    public Mesh mesh;
    public float scale = 1.0f;
    private CommandBuffer m_CommandBuffer;
    public Material m_ForceMaterial;

    void create_camera()
    {
        Bounds box = mesh.bounds;
        float width = box.extents.x * 2 * scale;
        float height = box.extents.z * 2 * scale;
        float depth = 10.0f;
        Debug.Log(string.Format("in create camera get extents:{0} {1}", width, height));

        m_Camera = gameObject.AddComponent<Camera>();
        m_Camera.aspect = width / height;
        m_Camera.backgroundColor = Color.black;
        m_Camera.cullingMask = 0; // 1 << 0;
        m_Camera.depth = 0;
        m_Camera.farClipPlane = depth;
        m_Camera.nearClipPlane = -10;
        m_Camera.orthographic = true;
        m_Camera.orthographicSize = height * 0.5f;
        m_Camera.clearFlags = CameraClearFlags.SolidColor;
        m_Camera.allowHDR = false;

        Camera cam = this.gameObject.GetComponent<Camera>();
        if (cam == null)
            cam = this.gameObject.AddComponent<Camera>();
        cam = m_Camera;
    }

    void init_rt()
    {
        rt = new RenderTexture(512, 512, 0, RenderTextureFormat.ARGB32);
        m_Camera.targetTexture = rt;

        m_CommandBuffer = new CommandBuffer();
        m_Camera.AddCommandBuffer(CameraEvent.AfterImageEffectsOpaque, m_CommandBuffer);
    }

    void DebugPanel()
    {
        RawImage img = GameObject.Find("ui/show_rt").GetComponent<RawImage>();
        img.texture = rt;
    }

    // Use this for initialization
    void Start () {
        create_camera();
        init_rt();
        DebugPanel();
	}

    public void ForceDrawMesh(Mesh mesh, Matrix4x4 matrix)
    {
        if (!mesh)
            return;
        //if (IsBoundsInCamera(mesh.bounds, m_Camera))
        m_CommandBuffer.DrawMesh(mesh, matrix, m_ForceMaterial);
    }

    // Update is called once per frame
    void Update () {
        ForceDrawMesh(mesh, Matrix4x4.identity);
    }

    void OnPostRender()
    {
        m_CommandBuffer.Clear();
        m_CommandBuffer.ClearRenderTarget(true, false, Color.black);
    }
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

public class shadowmap_camera : MonoBehaviour {
    public Light dir_light;
    public int shadowmap_size = 1024;
    static int shadowmap_rt_id = Shader.PropertyToID("shadowmap_rt");
    public GameObject draw_mesh;
    public RenderTexture shadowmap_rt;

    private Camera m_camera;
    private CommandBuffer shadowmap_buffer;
    private Material shadowmap_mtl;
    

    // Use this for initialization
    void Start () {
        m_camera = gameObject.AddComponent<Camera>();

        m_camera.orthographic = true;
        m_camera.orthographicSize = 5.0f;
        m_camera.cullingMask = 0;
        m_camera.depth = -1;
        m_camera.clearFlags = CameraClearFlags.SolidColor;
        m_camera.backgroundColor = Color.black;
        m_camera.nearClipPlane = 0.1f;
        m_camera.farClipPlane = 60.0f;

        shadowmap_rt = new RenderTexture(shadowmap_size, shadowmap_size, 24);       
        Shader.SetGlobalTexture(shadowmap_rt_id, shadowmap_rt);

        shadowmap_mtl = new Material(Shader.Find("my_shader/shadowmap_gen"));
        shadowmap_buffer = new CommandBuffer();
        shadowmap_buffer.name = "shadowmap gen";
        shadowmap_buffer.SetRenderTarget(shadowmap_rt);
        shadowmap_buffer.ClearRenderTarget(true, true, Color.black);
        Mesh mesh = draw_mesh.GetComponent<MeshFilter>().mesh;
        shadowmap_buffer.DrawMesh(mesh, draw_mesh.transform.localToWorldMatrix, shadowmap_mtl);

        m_camera.AddCommandBuffer(CameraEvent.BeforeForwardOpaque, shadowmap_buffer);
    }

    private void OnEnable()
    {
    }

    void ClearShadowmapBuffer()
    {
        shadowmap_buffer.ReleaseTemporaryRT(shadowmap_rt_id);
        m_camera.RemoveCommandBuffer(CameraEvent.AfterForwardOpaque, shadowmap_buffer);
        shadowmap_buffer.Release();
        shadowmap_buffer.Dispose();
    }

    private void OnDestroy()
    {
        ClearShadowmapBuffer();
    }

    // Update is called once per frame
    void Update () {
        m_camera.transform.position = dir_light.transform.position;
        m_camera.transform.rotation = dir_light.transform.rotation;
        Shader.SetGlobalMatrix(Shader.PropertyToID("S_LightViewProjector"), m_camera.previousViewProjectionMatrix);
    }
}

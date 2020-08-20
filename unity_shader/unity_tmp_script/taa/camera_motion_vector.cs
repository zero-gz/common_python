using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class camera_motion_vector : MonoBehaviour {
    private Camera _camera;
    private test_frust_jitter _frustumJitter;

    private RenderTexture velocityBuffer;
    private bool paramInitialized = false;
    private Vector4 paramProjectionExtents;
    private Matrix4x4 paramCurrV;
    private Matrix4x4 paramCurrVP;
    private Matrix4x4 paramPrevVP;
    private Matrix4x4 paramPrevVP_NoFlip;

    private Material velocityMaterial;

    public RenderTexture activeVelocityBuffer { get { return velocityBuffer; } }
    // Use this for initialization
    void Start () {
        _camera = GetComponent<Camera>();
        _frustumJitter = GetComponent<test_frust_jitter>();

        velocityMaterial = new Material(Shader.Find("Playdead/Post/VelocityBuffer"));

        velocityBuffer = new RenderTexture(Screen.width, Screen.height, 16, RenderTextureFormat.RGFloat);
        velocityBuffer.filterMode = FilterMode.Point;
    }

    // Update is called once per frame
    void Update () {
		
	}

    public void DrawFullscreenQuad()
    {
        GL.PushMatrix();
        GL.LoadOrtho();
        GL.Begin(GL.QUADS);
        {
            GL.MultiTexCoord2(0, 0.0f, 0.0f);
            GL.Vertex3(0.0f, 0.0f, 0.0f); // BL

            GL.MultiTexCoord2(0, 1.0f, 0.0f);
            GL.Vertex3(1.0f, 0.0f, 0.0f); // BR

            GL.MultiTexCoord2(0, 1.0f, 1.0f);
            GL.Vertex3(1.0f, 1.0f, 0.0f); // TR

            GL.MultiTexCoord2(0, 0.0f, 1.0f);
            GL.Vertex3(0.0f, 1.0f, 0.0f); // TL
        }
        GL.End();
        GL.PopMatrix();
    }

    void OnPostRender()
    {
        int bufferW = _camera.pixelWidth;
        int bufferH = _camera.pixelHeight;

        {
            Matrix4x4 currV = _camera.worldToCameraMatrix;
            Matrix4x4 currP = GL.GetGPUProjectionMatrix(_camera.projectionMatrix, true);
            Matrix4x4 currP_NoFlip = GL.GetGPUProjectionMatrix(_camera.projectionMatrix, false);
            Matrix4x4 prevV = paramInitialized ? paramCurrV : currV;

            paramInitialized = true;
            paramProjectionExtents = _frustumJitter.enabled ? _camera.GetProjectionExtents(_frustumJitter.activeSample.x, _frustumJitter.activeSample.y) : _camera.GetProjectionExtents();
            paramCurrV = currV;
            paramCurrVP = currP * currV;
            paramPrevVP = currP * prevV;
            paramPrevVP_NoFlip = currP_NoFlip * prevV;
        }

        RenderTexture activeRT = RenderTexture.active;
        RenderTexture.active = velocityBuffer;
        {
            GL.Clear(true, true, Color.black);

            const int kPrepass = 0;
            const int kVertices = 1;
            const int kVerticesSkinned = 2;

            // 0: prepass
            velocityMaterial.SetVector("_ProjectionExtents", paramProjectionExtents);
            velocityMaterial.SetMatrix("_CurrV", paramCurrV);
            velocityMaterial.SetMatrix("_CurrVP", paramCurrVP);
            velocityMaterial.SetMatrix("_PrevVP", paramPrevVP);
            velocityMaterial.SetMatrix("_PrevVP_NoFlip", paramPrevVP_NoFlip);

            velocityMaterial.SetPass(kPrepass);
            DrawFullscreenQuad();

            // 1 + 2: vertices + vertices skinned
            var obs = VelocityBufferTag.activeObjects;
            for (int i = 0, n = obs.Count; i != n; i++)
            {
                var ob = obs[i];
                if (ob != null && ob.rendering && ob.mesh != null)
                {
                    velocityMaterial.SetMatrix("_CurrM", ob.localToWorldCurr);
                    velocityMaterial.SetMatrix("_PrevM", ob.localToWorldPrev);
                    velocityMaterial.SetPass(ob.meshSmrActive ? kVerticesSkinned : kVertices);

                    for (int j = 0; j != ob.mesh.subMeshCount; j++)
                    {
                        Graphics.DrawMeshNow(ob.mesh, Matrix4x4.identity, j);
                    }
                }
            }

        }
        RenderTexture.active = activeRT;
    }
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using SDFr;

[RequireComponent(typeof(Camera))]
public class test_mesh_sdf : MonoBehaviour
{
    private Camera m_cam;
    private Light m_light;
    private Material mesh_sdf_mtl;

    public SDFData volumeA;
    public Transform volumeATransform;

    public Vector4 sphere1_pos_r = new Vector4(0, 0, 0, 1);
    public Vector4 sphere2_pos_r = new Vector4(0, 1, 0, 1);
    public Vector4 box_pos_size = new Vector4(2, 1, 1, 1);
    public Vector4 plane_pos_y_offset = new Vector4(0,0,0,1);

    private VolumeData[] _volumesData;
    private ComputeBuffer _volumes;

    private const int VolumeDataStride = 76;
    private struct VolumeData
    {
        public Matrix4x4 WorldToLocal;
        public Vector3 Extents;
    }

    public static void SetupRaymarchingMatrix(float fieldOfView, Matrix4x4 view, Vector2 screenSize)
    {
        Vector4 screenSizeParams = new Vector4(screenSize.x, screenSize.y, 1.0f / screenSize.x, 1.0f / screenSize.y);

        Matrix4x4 pixelCoordToWorldSpaceViewDir = ComputePixelCoordToWorldSpaceViewDirectionMatrix(
            fieldOfView * Mathf.Deg2Rad,
            Vector2.zero,
            screenSizeParams,
            view,
            false
        );
        Shader.SetGlobalMatrix(Shader.PropertyToID("_PixelCoordToViewDirWS"), pixelCoordToWorldSpaceViewDir);
    }

    //From HDRP
    public static Matrix4x4 ComputePixelCoordToWorldSpaceViewDirectionMatrix(float verticalFoV, Vector2 lensShift, Vector4 screenSize, Matrix4x4 worldToViewMatrix, bool renderToCubemap)
    {
        // Compose the view space version first.
        // V = -(X, Y, Z), s.t. Z = 1,
        // X = (2x / resX - 1) * tan(vFoV / 2) * ar = x * [(2 / resX) * tan(vFoV / 2) * ar] + [-tan(vFoV / 2) * ar] = x * [-m00] + [-m20]
        // Y = (2y / resY - 1) * tan(vFoV / 2)      = y * [(2 / resY) * tan(vFoV / 2)]      + [-tan(vFoV / 2)]      = y * [-m11] + [-m21]

        float tanHalfVertFoV = Mathf.Tan(0.5f * verticalFoV);
        float aspectRatio = screenSize.x * screenSize.w;

        // Compose the matrix.
        float m21 = (1.0f - 2.0f * lensShift.y) * tanHalfVertFoV;
        float m11 = -2.0f * screenSize.w * tanHalfVertFoV;

        float m20 = (1.0f - 2.0f * lensShift.x) * tanHalfVertFoV * aspectRatio;
        float m00 = -2.0f * screenSize.z * tanHalfVertFoV * aspectRatio;

        if (renderToCubemap)
        {
            // Flip Y.
            m11 = -m11;
            m21 = -m21;
        }

        var viewSpaceRasterTransform = new Matrix4x4(new Vector4(m00, 0.0f, 0.0f, 0.0f),
            new Vector4(0.0f, m11, 0.0f, 0.0f),
            new Vector4(m20, m21, -1.0f, 0.0f),
            new Vector4(0.0f, 0.0f, 0.0f, 1.0f));

        // Remove the translation component.
        var homogeneousZero = new Vector4(0, 0, 0, 1);
        worldToViewMatrix.SetColumn(3, homogeneousZero);

        // Flip the Z to make the coordinate system left-handed.
        worldToViewMatrix.SetRow(2, -worldToViewMatrix.GetRow(2));

        // Transpose for HLSL.
        return Matrix4x4.Transpose(worldToViewMatrix.transpose * viewSpaceRasterTransform);
    }

    // Start is called before the first frame update
    void Start()
    {
        m_cam = GetComponent<Camera>();
        m_light = GameObject.Find("Directional Light").GetComponent<Light>();
        mesh_sdf_mtl = new Material(Shader.Find("Custom/mesh_sdf"));

        _volumesData = new VolumeData[2];
        _volumes = new ComputeBuffer(2, VolumeDataStride);
        _volumes.SetData(_volumesData);
    }

    void update_material_vars()
    {
        mesh_sdf_mtl.SetVector("_CamPosition", m_cam.transform.position);
        mesh_sdf_mtl.SetVector("_LightDir", m_light.transform.forward);
        mesh_sdf_mtl.SetVector("_LightPos", m_light.transform.position);

        mesh_sdf_mtl.SetVector("_Sphere1", sphere1_pos_r);
        mesh_sdf_mtl.SetVector("_Sphere2", sphere2_pos_r);
        mesh_sdf_mtl.SetVector("_box", box_pos_size);
        mesh_sdf_mtl.SetVector("_plane", plane_pos_y_offset);

        SetupRaymarchingMatrix(m_cam.fieldOfView, m_cam.worldToCameraMatrix, new Vector2(m_cam.pixelWidth, m_cam.pixelHeight));

        _volumesData[0].WorldToLocal = volumeATransform.worldToLocalMatrix;
        _volumesData[0].Extents = volumeA.bounds.extents;
        _volumes.SetData(_volumesData);

        mesh_sdf_mtl.SetBuffer("_VolumeBuffer", _volumes);
        mesh_sdf_mtl.SetTexture("_VolumeATex", volumeA.sdfTexture);
    }

    // Update is called once per frame
    void Update()
    {
        update_material_vars();
    }

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        Graphics.Blit(source, destination, mesh_sdf_mtl);
    }
}

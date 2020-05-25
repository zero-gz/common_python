using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

[CustomEditor(typeof(preinteger_assets))]
public class preinteger_property : Editor {
    private preinteger_assets assets;
    private Texture2D tex;
    private Material graphMaterial;
    private Material radiusMaterial;

    void updateMaterial()
    {
        graphMaterial.SetVector("factor1", new Vector4(assets.factor1.r, assets.factor1.g, assets.factor1.b, assets.factor1.a));
        graphMaterial.SetVector("factor2", new Vector4(assets.factor2.r, assets.factor2.g, assets.factor2.b, assets.factor2.a));
        graphMaterial.SetVector("factor3", new Vector4(assets.factor3.r, assets.factor3.g, assets.factor3.b, assets.factor3.a));
        graphMaterial.SetVector("factor4", new Vector4(assets.factor4.r, assets.factor4.g, assets.factor4.b, assets.factor4.a));
        graphMaterial.SetVector("factor5", new Vector4(assets.factor5.r, assets.factor5.g, assets.factor5.b, assets.factor5.a));
        graphMaterial.SetVector("factor6", new Vector4(assets.factor6.r, assets.factor6.g, assets.factor6.b, assets.factor6.a));
        graphMaterial.SetFloat("_x_size", assets.x_size);
        graphMaterial.SetFloat("_y_size", assets.y_size);

        radiusMaterial.SetVector("factor1", new Vector4(assets.factor1.r, assets.factor1.g, assets.factor1.b, assets.factor1.a));
        radiusMaterial.SetVector("factor2", new Vector4(assets.factor2.r, assets.factor2.g, assets.factor2.b, assets.factor2.a));
        radiusMaterial.SetVector("factor3", new Vector4(assets.factor3.r, assets.factor3.g, assets.factor3.b, assets.factor3.a));
        radiusMaterial.SetVector("factor4", new Vector4(assets.factor4.r, assets.factor4.g, assets.factor4.b, assets.factor4.a));
        radiusMaterial.SetVector("factor5", new Vector4(assets.factor5.r, assets.factor5.g, assets.factor5.b, assets.factor5.a));
        radiusMaterial.SetVector("factor6", new Vector4(assets.factor6.r, assets.factor6.g, assets.factor6.b, assets.factor6.a));
    }

    void OnEnable()
    {
        //获取当前编辑自定义Inspector的对象
        assets = (preinteger_assets)target;
        tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Resources/preinteger_diffusion_profile.png");

        graphMaterial = new Material(Shader.Find("my_shader/draw_graph_3color_guass"));
        radiusMaterial = new Material(Shader.Find("my_shader/draw_diffuse_scatter"));

        updateMaterial();
    }

    float Gaussian(float v, float r)
    {
        return 1.0f / Mathf.Sqrt(2.0f * Mathf.PI * v) * Mathf.Exp(-(r * r) / (2.0f * v));
        //return 1.0f / (2.0f * Mathf.PI * v) * Mathf.Exp(-(r * r) / (2.0f * v));
    }

    Vector3 Scatter(float r)
    {
        float sq = 1.414f;

        return Gaussian(assets.factor1.r * sq, r) * new Vector3(assets.factor1.g, assets.factor1.b, assets.factor1.a) +
            Gaussian(assets.factor2.r * sq, r) * new Vector3(assets.factor2.g, assets.factor2.b, assets.factor2.a) +
            Gaussian(assets.factor3.r * sq, r) * new Vector3(assets.factor3.g, assets.factor3.b, assets.factor3.a) +
            Gaussian(assets.factor4.r * sq, r) * new Vector3(assets.factor4.g, assets.factor4.b, assets.factor4.a) +
            Gaussian(assets.factor5.r * sq, r) * new Vector3(assets.factor5.g, assets.factor5.b, assets.factor5.a) +
            Gaussian(assets.factor6.r * sq, r) * new Vector3(assets.factor6.g, assets.factor6.b, assets.factor6.a);
    }

    Vector3 integrateDiffuseScatteringOnRing(float cosTheta, float skinRadius, float inc)
    {
        //float theta = Mathf.Acos(cosTheta);
        float theta = Mathf.PI * (1.0f - cosTheta);
        Vector3 totalWeights = new Vector3(0.0f, 0.0f, 0.0f);
        Vector3 totalLight = new Vector3(0.0f, 0.0f, 0.0f);
        float x = -(Mathf.PI / 2);
        while (x <= (Mathf.PI / 2))
        {
            float diffuse = Mathf.Clamp01(Mathf.Cos(theta + x));
            float sampleDist = Mathf.Abs(2.0f * skinRadius * Mathf.Sin(x * 0.5f));
            Vector3 weights = Scatter(sampleDist);
            totalWeights += weights;
            totalLight += diffuse * weights;
            x += inc;
        }
        Vector3 result;
        result.x = totalLight.x / totalWeights.x;
        result.y = totalLight.y / totalWeights.y;
        result.z = totalLight.z / totalWeights.z;
        return result;
    }

    void _gen_preinteger_tex()
    {
        int width, height;
        width = height = assets.tex_size;
        Texture2D tex = new Texture2D(width, height);

        for (int j = 0; j < height; j++)
            for (int i = 0; i < width; i++)
            {
                float cosTheta = i * 1.0f / width;
                float height_y = j * 1.0f / height;
                float skinRadius;
                if (height_y < 0.01f)
                    skinRadius = 100.0f;
                else
                    skinRadius = 1.0f / height_y;

                Vector3 result = integrateDiffuseScatteringOnRing(cosTheta, skinRadius, 0.1f);
                Color col = new Color(result.x, result.y, result.z);
                tex.SetPixel(i, j, col);
            }

        tex.Apply();
        util.save_texture(assets.tex_path + "preinteger.png", tex);
    }
 

    public override void OnInspectorGUI()
    {
        EditorGUI.BeginChangeCheck();
        {
            base.OnInspectorGUI();
        }
        if (EditorGUI.EndChangeCheck())
        {
            Debug.Log("get something changed!!!!!!!!!!!!!");
            updateMaterial();
        }

        if (GUILayout.Button("生成预积分贴图"))
        {
            Debug.Log(string.Format("get button click!!!"));
            _gen_preinteger_tex();
        }

        EditorGUILayout.Space();
        EditorGUILayout.LabelField("--------- 下面是添加的显示部分 -------------");

        EditorGUILayout.LabelField("       高斯曲线                                                          示意图");
        Vector2 start_pos = new Vector2(40, 330);
        Vector2 pic_size = new Vector2(256, 256);

        Rect rect = new Rect(start_pos, pic_size);
        Graphics.DrawTexture(rect, EditorGUIUtility.whiteTexture, graphMaterial);

        start_pos.x += 300;
        Rect rect1 = new Rect(start_pos, pic_size);
        Graphics.DrawTexture(rect1, EditorGUIUtility.whiteTexture, radiusMaterial);
    }

    /*
    public override Texture2D RenderStaticPreview
    (
        string assetPath,
        Object[] subAssets,
        int width,
        int height
    )
    {
        var obj = target as preinteger_assets;
        if (tex == null)
        {
            return base.RenderStaticPreview(assetPath, subAssets, width, height);
        }

        var preview = AssetPreview.GetAssetPreview(tex);
        var final = new Texture2D(width, height);

        EditorUtility.CopySerialized(preview, tex);

        return final;
    }

    */

        /*
    public override bool HasPreviewGUI()
    {
        return true;
    }

    public override void OnPreviewSettings()
    {
        GUILayout.Label("文本", "preLabel");
        GUILayout.Button("按钮", "preButton");
    }

    public override void OnPreviewGUI(Rect r, GUIStyle background)
    {
        GUI.DrawTexture(r, tex, ScaleMode.StretchToFill, false);
    }
    */


        /*
        public override Texture2D RenderStaticPreview(string assetPath, Object[] subAssets, int width, int height)
        {
            Texture2D tex = null;

            preinteger_assets profile = (preinteger_assets)AssetDatabase.LoadAssetAtPath(assetPath, typeof(preinteger_assets));

            bool supportsRenderTextures;
    #if UNITY_5_5_OR_NEWER
            // From 5.5 on SystemInfo.supportsRenderTextures always returns true and Unity produces an ugly warning.
            supportsRenderTextures = true;
    #else
                supportsRenderTextures = SystemInfo.supportsRenderTextures;
    #endif

            if (profile != null && supportsRenderTextures)
            {
                try
                {
                    // for static preview always show the radial distance, which visualizes well the profile, even with small size and just looks good :)
                    Material mat = new Material(Shader.Find("my_shader/draw_graph_guass"));
                    try
                    {
                        mat.hideFlags = HideFlags.HideAndDontSave;

                        RenderTexture outRT = new RenderTexture(width, height, 0, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Linear);
                        try
                        {
                            if (outRT.Create())
                            {
                                RenderTexture oldRndT = RenderTexture.active;
                                RenderTexture.active = outRT;
                                try
                                {
                                    mat.SetFloat("_TextureWidth", width);
                                    mat.SetFloat("_TextureHeight", height);

                                    GL.PushMatrix();
                                    GL.LoadOrtho();

                                    for (int i = 0; i < mat.passCount; i++)
                                    {
                                        mat.SetPass(i);

                                        GL.Begin(GL.QUADS);

                                        GL.TexCoord2(0f, 1f);
                                        GL.Vertex3(0f, 0f, 0);

                                        GL.TexCoord2(1f, 1f);
                                        GL.Vertex3(1f, 0f, 0);

                                        GL.TexCoord2(1f, 0f);
                                        GL.Vertex3(1f, 1f, 0);

                                        GL.TexCoord2(0f, 0f);
                                        GL.Vertex3(0f, 1f, 0);

                                        GL.End();
                                    }

                                    GL.PopMatrix();

                                    tex = new Texture2D(width, height, TextureFormat.ARGB32, true, true);
                                    tex.ReadPixels(new Rect(0, 0, width, height), 0, 0, true);

                                    tex.Apply();
                                }
                                finally
                                {
                                    RenderTexture.active = oldRndT;
                                    Graphics.SetRenderTarget(oldRndT);

                                    outRT.Release();
                                }
                            }
                        }
                        finally
                        {
                            DestroyImmediate(outRT);
                        }
                    }
                    finally
                    {
                        DestroyImmediate(mat);
                    }
                }
                catch (System.Exception ex)
                {
                    tex = null;
                    Debug.LogException(ex);
                }
            }

            // fall back to simple icon
            if (tex == null)
                base.RenderStaticPreview(assetPath, subAssets, width, height);

            return tex;
        }*/

    }

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

[CustomEditor(typeof(preinteger_assets))]
public class preinteger_property : Editor {
    private preinteger_assets assets;
    private Texture2D tex;

    void OnEnable()
    {
        //获取当前编辑自定义Inspector的对象
        assets = (preinteger_assets)target;
        tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Resources/preinteger_diffusion_profile.png");
    }
    /*
        EditorGUI.BeginChangeCheck();
        {
            assets.value = EditorGUILayout.Slider("this is a slider", assets.value, 0, 1);
        }
        if (EditorGUI.EndChangeCheck())
        {
            Debug.Log(string.Format("get slider value:{0}", assets.value ));
        }
     */

    public override void OnInspectorGUI()
    {
        base.OnInspectorGUI();
        EditorGUILayout.Space();
        EditorGUILayout.LabelField("--------- 下面是添加的显示部分 -------------");

        Vector2 start_pos = new Vector2(20, 230);
        Vector2 pic_size = new Vector2(150, 150);

        Rect rect = new Rect(start_pos, pic_size);
        Material previewMaterial = new Material(Shader.Find("my_shader/draw_graph_guass"));
        previewMaterial.SetColor("_line_color", new Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        Graphics.DrawTexture(rect, EditorGUIUtility.whiteTexture, previewMaterial);


        start_pos.y += 200;
        Rect rect1 = new Rect(start_pos, pic_size);
        Material previewMaterial1 = new Material(Shader.Find("my_shader/draw_graph_guass"));
        previewMaterial1.SetColor("_line_color", new Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        previewMaterial1.SetInt("_line_num", 1);
        Graphics.DrawTexture(rect1, EditorGUIUtility.whiteTexture, previewMaterial1);

        for(int i=0;i<65;i++)
            EditorGUILayout.Space();

        if (GUILayout.Button("生成 diffuse_povisor贴图"))
        {
            Debug.Log(string.Format("get button click!!!"));
        }
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

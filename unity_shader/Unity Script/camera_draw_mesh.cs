using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class camera_draw_mesh : MonoBehaviour {

	// Use this for initialization
	void Start () {
        Debug.Log("call start ok!!!!!!!!!");
	}
	
	// Update is called once per frame
	void Update () {
        Debug.Log("call update ok!!!!!!!!!!!");
        /*
        int width = 1024;
        int height = 1024;
        RenderTexture rt = new RenderTexture(width, height, 0, RenderTextureFormat.ARGB32);
        Graphics.SetRenderTarget(rt);

        Mesh mesh = AssetDatabase.LoadAssetAtPath<Mesh>("Assets/Scenes/my_shader_template/mesh_res/jl/women_body_jl.fbx");
        if (!mesh)
            Debug.LogError("can not load mesh!!!!!!!!!!");

        Material mtl = new Material(Shader.Find("my_shader/test_light_uv"));
        Graphics.DrawMesh(mesh, Matrix4x4.identity, mtl, 0);

        Texture2D result = new Texture2D(width, height, TextureFormat.ARGB32, false);
        result.ReadPixels(new Rect(0, 0, width, height), 0, 0);
        result.Apply();

        byte[] data = result.EncodeToPNG();
        Debug.Log(string.Format("get png length {0}", data.Length));
        System.IO.File.WriteAllBytes("Assets/Resources/test_light_uv.png", data);

        Graphics.SetRenderTarget(null);
        rt.Release();
        AssetDatabase.Refresh();
        */
    }

    void OnPostRender()
    {
    }
}

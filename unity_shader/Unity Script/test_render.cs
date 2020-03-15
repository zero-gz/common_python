using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class test_render : MonoBehaviour {
    public void build_uv_map()
    {
        MeshFilter mf = GetComponent<MeshFilter>();
        Material mtl = GetComponent<Renderer>().material;
        int width = 512;
        int height = 512;

        Material old_mtl = mtl;

        Material new_mtl = new Material(Shader.Find("my_shader/test_uv"));

        Texture2D _tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Scenes/my_shader_template/mesh_res/gf/body_gf_01_d.dds");
        if (!_tex)
            Debug.LogError("can not find texture!");

        new_mtl.SetTexture("_MainTex", _tex);
        new_mtl.SetPass(0);
        mtl = new_mtl;

        RenderTexture rt = new RenderTexture(width, height, 0, RenderTextureFormat.ARGB32);
        Graphics.SetRenderTarget(rt);
        Graphics.DrawMeshNow(mf.mesh, Matrix4x4.identity);

        Texture2D result = new Texture2D(width, height, TextureFormat.ARGB32, false);
        result.ReadPixels(new Rect(0, 0, width, height), 0, 0);
        result.Apply();

        byte[] data = result.EncodeToPNG();
        Debug.Log(string.Format("get png length {0}", data.Length));
        System.IO.File.WriteAllBytes("Assets/Resources/test_uv.png", data);

        Graphics.SetRenderTarget(null);
        rt.Release();
        AssetDatabase.Refresh();
        GetComponent<Renderer>().material = old_mtl;
    }

	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}

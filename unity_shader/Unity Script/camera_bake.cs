using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class camera_bake : MonoBehaviour {
    bool grab = false;

    static void build_uv_map()
    {
        GameObject body = GameObject.Find("women_body_gl");
        MeshFilter mf = body.GetComponent<MeshFilter>();
        Material mtl = body.GetComponent<Renderer>().material;
        int width = 1024;
        int height = 1024;

        Material new_mtl = new Material(Shader.Find("my_shader/test_light_uv"));
        //mtl.shader = Shader.Find("my_shader/test_uv");
        //mtl.SetPass(0);

        Material old_mtl = mtl;

        Texture2D _tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Scenes/my_shader_template/mesh_res/gl/body_gl_01_d.dds");
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

        body.GetComponent<Renderer>().material = old_mtl;
    }

    private void Start()
    {
        
    }

    private void Update()
    {
        //Press space to start the screen grab
        if (Input.GetKeyDown(KeyCode.Space))
        {
            grab = true;
            Debug.Log("get space key down!");
        }
    }

    private void OnPreRender()
    {
        
    }

    private void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        Debug.Log("Get on Render image call!!!");
        Graphics.Blit(source, destination);
    }

    private void OnPostRender()
    {
        Debug.Log("get on post render call!!!");
        if (grab)
        {
            //Create a new texture with the width and height of the screen
            Texture2D texture = new Texture2D(Screen.width, Screen.height, TextureFormat.RGB24, false);
            //Read the pixels in the Rect starting at 0,0 and ending at the screen's width and height
            texture.ReadPixels(new Rect(0, 0, Screen.width, Screen.height), 0, 0, false);
            texture.Apply();

            byte[] data = texture.EncodeToPNG();
            Debug.Log(string.Format("get png length {0}", data.Length));
            System.IO.File.WriteAllBytes("Assets/Resources/camera_render.png", data);
            //Reset the grab state
            grab = false;
        }
    }
}

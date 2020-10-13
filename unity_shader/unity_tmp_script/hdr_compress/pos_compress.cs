using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class pos_compress : MonoBehaviour {
    public Texture2D exr_tex;
    public string save_path = "Assets/pos_compress/compress.png";
    public enum CompressType {RGBE, RGBM, LOGUV};
    public CompressType c_type;

    public Color TransformColor(Color exr_color)
    {
        Color ret_color = new Color();
        switch(c_type)
        {
            case CompressType.RGBE:
                Debug.Log("get rgbe ok!");
                break;
            case CompressType.RGBM:
                Debug.Log("get rgbm ok!");
                break;
            case CompressType.LOGUV:
                Debug.Log("get loguv ok!");
                break;
        }
        return ret_color;
    }

    public void trans_exr_to_png()
    {
        int width = exr_tex.width;
        int height = exr_tex.height;
        Debug.Log(string.Format("get width {0}, height {1}", width, height));
        Texture2D save_tex = new Texture2D(width, height, TextureFormat.ARGB32, false);
        for(int i=0;i<height;i++)
            for(int j=0;j<width;j++)
            {
                Color exr_color = save_tex.GetPixel(i, j);
                Debug.Log(string.Format("{0} {1} get Pixel data {2}", i, j, exr_color.ToString()));
                Color save_color = TransformColor(exr_color);
                save_tex.SetPixel(i, j, save_color);
            }

        save_tex.Apply();
        util.save_texture(save_path, save_tex);
    }

    public void create_exr_file()
    {
        int width = 5;
        int height = 5;
        Texture2D tex = new Texture2D(width, height, TextureFormat.RGBAHalf, false);

        Color color = new Color(-1.0f, 0.0f, 100.0f);
        tex.SetPixel(0, 0, color);
        util.save_texture("Assets/pos_compress/test.exr", tex);
    }

	// Use this for initialization
	void Start () {
        trans_exr_to_png();
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}

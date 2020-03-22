using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using System;

public class camera_paint : MonoBehaviour {
    private RenderTexture paint;
    private Texture2D paint_tex;
    private Vector2 last_point;

    public float paint_size = 5.0f;
    public float diffrence = 1.0f;
	// Use this for initialization
	void Start () {
        paint = new RenderTexture(512, 512, 0, RenderTextureFormat.ARGB32);
        GameObject obj = GameObject.Find("paint_plane");
        Material mtl = obj.GetComponent<Renderer>().material;

        paint_tex = new Texture2D(512, 512, TextureFormat.ARGB32, false);
        mtl.SetTexture("_MainTex", paint_tex);
	}

    void draw_texture(Vector2 mouse_pos)
    {
        Material mtl = AssetDatabase.LoadAssetAtPath<Material>("Assets/Scenes/test_script/test_paint.mat");
        mtl.SetVector("_paint", new Vector4(Screen.width - mouse_pos.x, Screen.height - mouse_pos.y, paint_size, 1.0f));
        paint = util.gpu_draw_rendertexture(paint, mtl);

        GameObject obj = GameObject.Find("paint_plane");
        Material obj_mtl = obj.GetComponent<Renderer>().material;
        obj_mtl.SetTexture("_MainTex", paint);
    }

    int Clamping(int x, int edage_x, int edage_y)
    {
        if (x <= edage_x) x = edage_x;
        if (x >= edage_y) x = edage_y;
        return x;
    }

    Color Lerp(Color one, Color two, float factor)
    {
        Color output = new Color();
        output.r = Mathf.Lerp(one.r, two.r, factor);
        output.g = Mathf.Lerp(one.g, two.g, factor);
        output.b = Mathf.Lerp(one.b, two.b, factor);
        output.a = Mathf.Lerp(one.a, two.a, factor);
        return output;
    }

    // 使用CPU方式来绘制
    void draw_texture_cpu(Vector2 mouse_pos)
    {
        int aim_x = (int)((Screen.width - mouse_pos.x) / Screen.width * 512);
        int aim_y = (int)((Screen.height - mouse_pos.y) / Screen.height * 512);

        int half_size = (int)(paint_size/2);
        for(int x =-half_size; x <= half_size; x++)
            for(int y = -half_size; y<=half_size; y++)
            {
                int tmp_x = aim_x + x;
                int tmp_y = aim_y + y;

                tmp_x = Clamping(tmp_x, 0, 512);
                tmp_y = Clamping(tmp_y, 0, 512);


                Color color = new Color(1.0f, 0.0f, 0.0f);
                Color org_color = paint_tex.GetPixel(tmp_x, tmp_y);

                float factor = (float)(Math.Sqrt(x * x + y * y)/half_size);
                Color final_color = Lerp(color, org_color, factor);

                paint_tex.SetPixel(tmp_x, tmp_y, color);
            }

        Debug.Log(string.Format("get paint color: {0}, {1}", aim_x, aim_y));
        paint_tex.Apply();

        GameObject obj = GameObject.Find("paint_plane");
        Material obj_mtl = obj.GetComponent<Renderer>().material;
        obj_mtl.SetTexture("_MainTex", paint_tex);
    }
	
	// Update is called once per frame
	void Update () {
        if (Input.GetMouseButton(0))
        {
            Vector2 now_point = Input.mousePosition;
            if (last_point == Vector2.zero) last_point = now_point;
            float dist = Vector2.Distance(now_point, last_point);
            int step = (int)(dist / diffrence);
            Vector2 dir = now_point - last_point;
            dir.Normalize();

            //Debug.Log(string.Format("we get the step {0}, diff_vec {1},{2}", step, dir.x, dir.y));
            
            for(int i=0;i<step;i++)
            {
                Vector2 aim_pos = last_point + dir*i;
                draw_texture_cpu(aim_pos); 
            }

            draw_texture_cpu(now_point);

            last_point = now_point;
        }

        if (Input.GetMouseButtonUp(0))
            last_point = Vector2.zero;
    }
}

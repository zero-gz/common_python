﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

using System.IO;
using System.Text;
using System;

// 制作这种通用类
public class util{

    public static void test_call()
    {
        Debug.Log("i want to call !!!!");
    }

    public static void save_texture(string save_path, Texture2D tex)
    {
        //if(tex.)

        byte[] data;

        if (tex.format == TextureFormat.ARGB32 || tex.format == TextureFormat.RGB24 || tex.format == TextureFormat.RGBA32)
            data = tex.EncodeToPNG();
        else if (tex.format == TextureFormat.RGBAHalf || tex.format == TextureFormat.RGBAFloat)
            data = tex.EncodeToEXR();
        else
        {
            Debug.LogError("texture is not support format!");
            return;
        }
        System.IO.File.WriteAllBytes(save_path, data);
        AssetDatabase.Refresh();
    }

    public static void save_screen_tex(string save_path, int start_x = 0, int start_y = 0, int width = 0,int height = 0)
    {
        if (width == 0) width = Screen.width;
        if (height == 0) height = Screen.height;

        Texture2D tex = new Texture2D(width, height, TextureFormat.ARGB32, false);
        Debug.Log(string.Format("get screen width and height: {0} {1}", Screen.width, Screen.height) );
        tex.ReadPixels(new Rect(start_x, start_y, width, height), 0, 0);
        tex.Apply();

        save_texture(save_path, tex);
    }

    public static void save_rendertexture(string save_path, RenderTexture rt)
    {
        RenderTexture prev = RenderTexture.active;
        RenderTexture.active = rt;

        int width = rt.width;
        int height = rt.height;

        Texture2D tex = new Texture2D(width, height, TextureFormat.ARGB32, false);
        Debug.Log(string.Format("get render texture width and height: {0} {1}", width, height));
        tex.ReadPixels(new Rect(0, 0, width, height), 0, 0);
        tex.Apply();

        save_texture(save_path, tex);

        RenderTexture.active = prev;
    }

    // -----------------------------------------------------------------------------

    public static string trans_mesh_to_string(MeshFilter mf, Vector3 scale)
    {
        Mesh mesh = mf.mesh;
        Vector2 textureOffset = new Vector2(0, 0);
        Vector2 textureScale = new Vector2(1, 1);
        StringBuilder stringBuilder = new StringBuilder().Append("mtllib design.mtl")
            .Append("\n")
            .Append("g ")
            .Append(mf.name)
            .Append("\n");
        Vector3[] vertices = mesh.vertices;
        for (int i = 0; i < vertices.Length; i++)
        {
            Vector3 vector = vertices[i];
            stringBuilder.Append(string.Format("v {0} {1} {2}\n", vector.x * scale.x, vector.y * scale.y, vector.z * scale.z));
        }
        stringBuilder.Append("\n");
        Dictionary<int, int> dictionary = new Dictionary<int, int>();
        if (mesh.subMeshCount > 1)
        {
            int[] triangles = mesh.GetTriangles(1);
            for (int j = 0; j < triangles.Length; j += 3)
            {
                if (!dictionary.ContainsKey(triangles[j]))
                {
                    dictionary.Add(triangles[j], 1);
                }
                if (!dictionary.ContainsKey(triangles[j + 1]))
                {
                    dictionary.Add(triangles[j + 1], 1);
                }
                if (!dictionary.ContainsKey(triangles[j + 2]))
                {
                    dictionary.Add(triangles[j + 2], 1);
                }
            }
        }

        int num = 0;
        for (num = 0; num != mesh.uv.Length; num++)
        {
            Vector2 vector2 = Vector2.Scale(mesh.uv[num], textureScale) + textureOffset;
            if (dictionary.ContainsKey(num))
            {
                stringBuilder.Append(string.Format("vt {0} {1}\n", mesh.uv[num].x, mesh.uv[num].y));
            }
            else
            {
                stringBuilder.Append(string.Format("vt {0} {1}\n", vector2.x, vector2.y));
            }
        }

        stringBuilder.Append("\n");
        for (num = 0; num != mesh.normals.Length; num++)
        {
            Vector3 normal = mesh.normals[num];
            stringBuilder.Append(string.Format("vn {0} {1} {2}\n", normal.x, normal.y, normal.z));
        }

        for (int k = 0; k < mesh.subMeshCount; k++)
        {
            stringBuilder.Append("\n");
            if (k == 0)
            {
                stringBuilder.Append("usemtl ").Append("Material_design").Append("\n");
            }
            if (k == 1)
            {
                stringBuilder.Append("usemtl ").Append("Material_logo").Append("\n");
            }
            int[] triangles2 = mesh.GetTriangles(k);
            for (int l = 0; l < triangles2.Length; l += 3)
            {
                stringBuilder.Append(string.Format("f {0}/{0} {1}/{1} {2}/{2}\n", triangles2[l] + 1, triangles2[l + 2] + 1, triangles2[l + 1] + 1));
            }
        }
        return stringBuilder.ToString();
    }

    public void save_mesh_to_obj(GameObject obj, string save_path)
    {
        Renderer renderer = obj.GetComponent<Renderer>();
        MeshFilter mf = renderer.GetComponent<MeshFilter>();

        StreamWriter streamWriter = new StreamWriter(save_path);
        streamWriter.Write(trans_mesh_to_string(mf, new Vector3(-1f, 1f, 1f)));
        streamWriter.Close();
        
        AssetDatabase.Refresh();
    }
    // -----------------------------------------------------------------------------------------------------------------
    public static Material change_material(GameObject obj, string shader_name)
    {
        Renderer renderer = obj.GetComponent<Renderer>();

        Material old_mtl = renderer.material;

        Material new_mtl = new Material(Shader.Find(shader_name));
        new_mtl.SetPass(0);
        renderer.material = new_mtl;

        return old_mtl;
    }

    // ------------------------------------------------------------------------------------------------------------------

    public static Mesh create_plane(int w_num, int h_num)
    {
        Mesh sub_mesh = new Mesh();

        int vertex_num = (w_num + 1) * (h_num + 1);
        int index_num = w_num * h_num * 2 * 3;
        Vector3[] vertices = new Vector3[vertex_num];
        Vector2[] uv = new Vector2[vertex_num];
        int[] indexs = new int[index_num];

        int i, j, count;
        i = j = count = 0;
        for (i = 0; i <= h_num; i++)
        {
            for (j = 0; j <= w_num; j++)
            {
                int index = (w_num + 1) * i + j;
                vertices[index] = new Vector3(j * 1.0f / w_num, i * 1.0f / h_num, 0);
                uv[index] = new Vector2(j * 1.0f / w_num, i * 1.0f / h_num);
            }
        }


        for (i = 0; i < h_num; i++)
        {
            for (j = 0; j < w_num; j++)
            {
                //产生两个三角形
                int tri_a_1 = (w_num + 1) * i + j;
                int tri_b_1 = tri_a_1 + 1;
                int tri_c_1 = (w_num + 1) * (i + 1) + (j + 1);

                int tri_a_2 = tri_a_1;
                int tri_b_2 = tri_c_1;
                int tri_c_2 = (w_num + 1) * (i + 1) + j;

                indexs[count] = tri_a_1;
                indexs[count + 1] = tri_b_1;
                indexs[count + 2] = tri_c_1;

                indexs[count + 3] = tri_a_2;
                indexs[count + 4] = tri_b_2;
                indexs[count + 5] = tri_c_2;

                count += 6;
            }
        }

        sub_mesh.vertices = vertices;
        sub_mesh.uv = uv;
        sub_mesh.triangles = indexs;

        /*
        string s = "";
        for (i = 0; i < vertex_num; i++)
            s += string.Format("({0}, {1}, {2}), ", vertices[i].x, vertices[i].y, vertices[i].z);

        s += "\n";
        for (i = 0; i < index_num; i++)
            s += string.Format("{0} ", indexs[i]);

        Debug.Log(s);
        */
        sub_mesh.RecalculateBounds();
        return sub_mesh;
    }

    // -------------------------------------------------------------------------

    public static RenderTexture gpu_draw_rendertexture(Texture src, Material mtl, int width=0, int height=0)
    {
        if (width == 0) width = src.width;
        if (height == 0) height = src.height;
        RenderTexture dst = new RenderTexture(width, height, 0, RenderTextureFormat.ARGB32);
        Graphics.Blit(src, dst, mtl);
        return dst;
    }
}


public class script_util : MonoBehaviour
{
    public bool grab = false;

    virtual protected void space_key_func()
    {
        Debug.Log("get space key down!");
    }

    protected void Update()
    {
        //Press space to start the screen grab
        if (Input.GetKeyDown(KeyCode.Space))
        {
            grab = true;
            space_key_func();
        }
    }
}

public class paint_util: MonoBehaviour
{
    protected RenderTexture paint;
    //protected Texture2D paint_tex;
    private Vector2 last_point;

    public float paint_size = 5.0f;
    public float diffrence = 1.0f;
    public int rt_size_x = 512;
    public int rt_size_y = 512;
    // Use this for initialization
    void Start()
    {
        paint = new RenderTexture(rt_size_x, rt_size_y, 0, RenderTextureFormat.ARGB32);
        //paint_tex = new Texture2D(rt_size_x, rt_size_y, TextureFormat.ARGB32, false);
    }

    void draw_texture(Vector2 mouse_pos)
    {
        Material mtl = AssetDatabase.LoadAssetAtPath<Material>("Assets/Scenes/test_script/test_paint.mat");
        mtl.SetVector("_paint", new Vector4(Screen.width - mouse_pos.x, Screen.height - mouse_pos.y, paint_size, 1.0f));
        paint = util.gpu_draw_rendertexture(paint, mtl);
    }

    /*
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

    void draw_texture_cpu(Vector2 mouse_pos)
    {
        int aim_x = (int)((Screen.width - mouse_pos.x) / Screen.width * rt_size_x);
        int aim_y = (int)((Screen.height - mouse_pos.y) / Screen.height * rt_size_y);

        int half_size = (int)(paint_size / 2);
        for (int x = -half_size; x <= half_size; x++)
            for (int y = -half_size; y <= half_size; y++)
            {
                int tmp_x = aim_x + x;
                int tmp_y = aim_y + y;

                tmp_x = Clamping(tmp_x, 0, rt_size_x);
                tmp_y = Clamping(tmp_y, 0, rt_size_y);


                Color color = new Color(1.0f, 0.0f, 0.0f);
                Color org_color = paint_tex.GetPixel(tmp_x, tmp_y);

                float factor = (float)(Math.Sqrt(x * x + y * y) / half_size);
                Color final_color = Lerp(color, org_color, factor);

                paint_tex.SetPixel(tmp_x, tmp_y, color);
            }

        Debug.Log(string.Format("get paint color: {0}, {1}", aim_x, aim_y));
        paint_tex.Apply();
    }
    */

    public virtual void after_draw_texture()
    {
        // paint rendertexture is ok, try to do something
        Debug.Log("call after draw texture, the paint rt is ok!");
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetMouseButton(0))
        {
            Vector2 now_point = Input.mousePosition;
            if (last_point == Vector2.zero) last_point = now_point;
            float dist = Vector2.Distance(now_point, last_point);
            int step = (int)(dist / diffrence);
            Vector2 dir = now_point - last_point;
            dir.Normalize();

            for (int i = 0; i < step; i++)
            {
                Vector2 aim_pos = last_point + dir * i;
                draw_texture(aim_pos);
            }

            draw_texture(now_point);
            after_draw_texture();

            last_point = now_point;
        }

        if (Input.GetMouseButtonUp(0))
            last_point = Vector2.zero;
    }
}
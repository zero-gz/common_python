using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class sphere_script : MonoBehaviour
{
    float PI = 3.1415926f;

    void print_object_mesh()
    {

    }

    Mesh create_sphere(float raidus, int w_num, int h_num)
    {
        Mesh sub_mesh = new Mesh();

        float theta; // -PI, PI
        float delta; // 0, 2*PI

        float d_delta = 2 * PI / w_num;
        float d_theta = 2 * PI / h_num;

        return sub_mesh;
    }

    //创建立方体

    //创建圆盘

    //创建圆柱体

    //创建球体 这个是不知道怎么做

    Mesh create_plane(int w_num, int h_num)
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
                vertices[index] = new Vector3(j *1.0f/ w_num, i*1.0f/h_num, 0);
                uv[index] = new Vector2(j*1.0f/w_num, i*1.0f/h_num);
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



    void init_material()
    {
        string texture_path = "block";
        Renderer renderer = gameObject.GetComponent<Renderer>();
        if (!renderer)
        {
            gameObject.AddComponent<MeshRenderer>();
            renderer = gameObject.GetComponent<Renderer>();
        }

        Texture img = Resources.Load<Texture>(texture_path);
        if (!img)
            Debug.Log("not find texture:" + texture_path);

        Material mtl = new Material(Shader.Find("my_shader/my_shader_tempalte"));
        Texture2D load_tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Shaders/my_shader_template/mesh_res/gf/body_gf_01_d.dds");
        mtl.SetTexture("_albedo_tex", load_tex);
        mtl.SetTexture("_id_tex", load_tex);

        renderer.material = mtl;
        AssetDatabase.CreateAsset(mtl, "Assets/Shaders/my_shader_template/test_mtl.mat");
    }

    void Start()
    {
        Mesh plane = create_plane(8, 5);

        MeshFilter filter = this.GetComponent<MeshFilter>();
        if (!filter)
        {
            gameObject.AddComponent<MeshFilter>();
            filter = this.GetComponent<MeshFilter>();
        }
        this.GetComponent<MeshFilter>().sharedMesh = plane;
        init_material();
    }

    // Update is called once per frame
    void Update()
    {
        //Debug.Log("this is update func!!");
    }
}

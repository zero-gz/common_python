﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using System.Text;
using UnityEditor;

public class util : MonoBehaviour {
    public string obj_save_path = "Assets/Resources/";
    string trans_mesh_to_string(MeshFilter mf, Vector3 scale)
    {
        Mesh mesh = mf.mesh;
        Material[] sharedMaterials = mf.GetComponent<Renderer>().sharedMaterials;
        Vector2 textureOffset = new Vector2(0, 0); // mf.GetComponent<Renderer>().material.GetTextureOffset("_MainTex");
        Vector2 textureScale = new Vector2(1, 1); // mf.GetComponent<Renderer>().material.GetTextureScale("_MainTex");
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

    public void save_obj()
    {
        Renderer renderer = gameObject.GetComponent<Renderer>();
        MeshFilter mf = renderer.GetComponent<MeshFilter>();

        using (StreamWriter streamWriter = new StreamWriter(string.Format("{0}{1}.obj", obj_save_path, mf.name)))
        {
            streamWriter.Write(trans_mesh_to_string(mf, new Vector3(-1f, 1f, 1f)));
            streamWriter.Close();
        }
        AssetDatabase.Refresh();
    }

	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}

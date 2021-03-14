using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;
using System.Globalization;

public class grass_mesh_process : ScriptableWizard
{
    public string _save_folder = "Assets";
    public bool has_pivot = true;
    public enum MaskType{PURE_BLACK, PURE_WHITE, SPHERE, HEIGHT_EXP};
    public MaskType mask_type = MaskType.HEIGHT_EXP; 

public GameObject prefabObject;


    private GameObject prefabInstance;

    private Vector4 maxBoundsInfo;


    private List<GameObject> gameObjectsInPrefab;
    private List<MeshFilter> meshFiltersInPrefab;
    private List<Mesh> meshesInPrefab;
    private List<Mesh> meshInstances;
    private string prefabDataFolder;

    void InitMaxBounds(Mesh mesh)
    {
        var bounds = mesh.bounds;

        var maxX = Mathf.Max(Mathf.Abs(bounds.min.x), Mathf.Abs(bounds.max.x));
        var maxZ = Mathf.Max(Mathf.Abs(bounds.min.z), Mathf.Abs(bounds.max.z));

        var maxR = Mathf.Max(maxX, maxZ);
        var maxH = Mathf.Max(Mathf.Abs(bounds.min.y), Mathf.Abs(bounds.max.y));
        var maxS = Mathf.Max(maxR, maxH);

        maxBoundsInfo = new Vector4(maxR, maxH, maxS, 0.0f);
        Debug.Log(string.Format("get max bounds info:{0}", maxBoundsInfo.ToString()));
    }

    // Get Vector2 data
    List<Vector4> GetCoordData(Mesh mesh, int source, int option)
    {
        var meshCoord = new List<Vector4>();

        if (source == 0)
        {
            mesh.GetUVs(0, meshCoord);
        }
        else if (source == 1)
        {
            meshCoord = GetCoordMeshData(mesh, option);
        }
        else if (source == 2)
        {
            meshCoord = GetCoordProceduralData(mesh, option);
        }

        if (meshCoord.Count == 0)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                meshCoord.Add(Vector4.zero);
            }
        }

        return meshCoord;
    }

    List<Vector4> GetCoordMeshData(Mesh mesh, int option)
    {
        var meshCoord = new List<Vector4>();

        if (option == 0)
        {
            mesh.GetUVs(0, meshCoord);
        }

        else if (option == 1)
        {
            mesh.GetUVs(1, meshCoord);
        }

        else if (option == 2)
        {
            mesh.GetUVs(2, meshCoord);
        }

        else if (option == 3)
        {
            mesh.GetUVs(3, meshCoord);
        }

        return meshCoord;
    }

    float MathRemap(float value, float low1, float high1, float low2, float high2)
    {
        return low2 + (value - low1) * (high2 - low2) / (high1 - low1);
    }

    List<float> GetMaskProceduralData(Mesh mesh, int option)
    {
        var meshChannel = new List<float>();

        if (option == 0)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                meshChannel.Add(0.0f);
            }
        }
        else if (option == 1)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                meshChannel.Add(1.0f);
            }
        }
        // Procedural Sphere
        else if (option == 2)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                var mask = Mathf.Clamp01(Vector3.Distance(mesh.vertices[i], Vector3.zero) / maxBoundsInfo.x);

                meshChannel.Add(mask);
            }
        }
        // Height Exp
        else if (option == 3)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                var oneMinusMask = 1 - Mathf.Clamp01(mesh.vertices[i].y / maxBoundsInfo.y);
                var powerMask = oneMinusMask * oneMinusMask * oneMinusMask * oneMinusMask;
                var mask = 1 - powerMask;

                meshChannel.Add(mask);
            }
        }

        return meshChannel;
    }

    List<Vector4> GetCoordProceduralData(Mesh mesh, int option)
    {
        var meshCoord = new List<Vector4>();

        // Planar XZ
        if (option == 0)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                meshCoord.Add(new Vector4(mesh.vertices[i].x, mesh.vertices[i].z, 0, 0));
            }
        }
        // Planar XY
        else if (option == 1)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                meshCoord.Add(new Vector4(mesh.vertices[i].x, mesh.vertices[i].y, 0, 0));
            }
        }
        // Planar ZY
        else if (option == 2)
        {
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                meshCoord.Add(new Vector4(mesh.vertices[i].z, mesh.vertices[i].y, 0, 0));
            }
        }
        // Procedural Pivots XZ
        else if (option == 3)
        {
            meshCoord = GenerateElementPivot(mesh);
        }

        return meshCoord;
    }

    List<Vector4> GenerateElementPivot(Mesh mesh)
    {
        var elementIndices = new List<int>();
        var meshPivots = new List<Vector4>();
        int elementCount = 0;

        for (int i = 0; i < mesh.vertexCount; i++)
        {
            elementIndices.Add(-99);
        }

        for (int i = 0; i < mesh.vertexCount; i++)
        {
            meshPivots.Add(Vector3.zero);
        }

        for (int i = 0; i < mesh.triangles.Length; i += 3)
        {
            var index1 = mesh.triangles[i + 0];
            var index2 = mesh.triangles[i + 1];
            var index3 = mesh.triangles[i + 2];

            int element = 0;

            if (elementIndices[index1] != -99)
            {
                element = elementIndices[index1];
            }
            else if (elementIndices[index2] != -99)
            {
                element = elementIndices[index2];
            }
            else if (elementIndices[index3] != -99)
            {
                element = elementIndices[index3];
            }
            else
            {
                element = elementCount;
                elementCount++;
            }

            elementIndices[index1] = element;
            elementIndices[index2] = element;
            elementIndices[index3] = element;
        }

        for (int e = 0; e < elementCount; e++)
        {
            var positions = new List<Vector3>();

            for (int i = 0; i < elementIndices.Count; i++)
            {
                if (elementIndices[i] == e)
                {
                    positions.Add(mesh.vertices[i]);
                }
            }

            float x = 0;
            float z = 0;

            for (int p = 0; p < positions.Count; p++)
            {
                x = x + positions[p].x;
                z = z + positions[p].z;
            }

            for (int i = 0; i < elementIndices.Count; i++)
            {
                if (elementIndices[i] == e)
                {
                    meshPivots[i] = new Vector3(x / positions.Count, z / positions.Count);
                }
            }
        }

        return meshPivots;
    }

    void ConvertMeshVegetation(Mesh mesh)
    {
        InitMaxBounds(mesh);

        Color[] colors = new Color[mesh.vertexCount];

        var UV0 = GetCoordMeshData(mesh, 0);

        if (has_pivot)
        {
            List<Vector4> detailCoordOrPivots = GetCoordProceduralData(mesh, 3);
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                UV0[i] = new Vector4(UV0[i].x, UV0[i].y, detailCoordOrPivots[i].x, detailCoordOrPivots[i].y);
            }
        }

        var motion1 = GetMaskProceduralData(mesh, (int)(mask_type) );
        for (int i = 0; i < mesh.vertexCount; i++)
        {
            colors[i].r = motion1[i];
        }

        mesh.colors = colors;
        mesh.SetUVs(0, UV0);
        SaveMesh(mesh, "_mod.mesh");
    }

    void CreatePrefabDataFolder()
    {
        var prefabPath = AssetDatabase.GetAssetPath(prefabObject);

        var folderPath = prefabPath.Replace(Path.GetFileName(prefabPath), "");
        folderPath = folderPath + prefabObject.name;

        if (Directory.Exists(folderPath) == false)
        {
            Directory.CreateDirectory(folderPath);
            AssetDatabase.Refresh();
        }

        prefabDataFolder = folderPath;
    }

    void SaveMesh(Mesh mesh, string suffix)
    {
        var saveFullPath = _save_folder + "/" + mesh.name + " " + suffix;
        Debug.Log(string.Format("get save path:{0}", saveFullPath));


        if (File.Exists(saveFullPath))
        {
            var adsMeshFile = AssetDatabase.LoadAssetAtPath<Mesh>(saveFullPath);
            adsMeshFile.Clear();
            EditorUtility.CopySerialized(mesh, adsMeshFile);
        }
        else
        {
            AssetDatabase.CreateAsset(mesh, saveFullPath);
        }

        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();
    }


    void GetGameObjectsInPrefab()
    {
        prefabInstance = Instantiate(prefabObject);

        gameObjectsInPrefab = new List<GameObject>();
        gameObjectsInPrefab.Add(prefabInstance);

        GetChildRecursive(prefabInstance);
    }

    void GetChildRecursive(GameObject go)
    {
        foreach (Transform child in go.transform)
        {
            if (child == null)
                continue;

            gameObjectsInPrefab.Add(child.gameObject);
            GetChildRecursive(child.gameObject);
        }
    }


    void GetMeshesInPrefab()
    {
        meshesInPrefab = new List<Mesh>();

        for (int i = 0; i < gameObjectsInPrefab.Count; i++)
        {
            if (meshFiltersInPrefab[i] != null)
            {
                meshesInPrefab.Add(meshFiltersInPrefab[i].sharedMesh);
            }
            else
            {
                meshesInPrefab.Add(null);
            }
        }
    }

    void CreateMeshInstances()
    {
        meshInstances = new List<Mesh>();

        for (int i = 0; i < gameObjectsInPrefab.Count; i++)
        {
            if (meshesInPrefab[i] != null)
            {
                var meshPath = AssetDatabase.GetAssetPath(meshesInPrefab[i]);

                if (meshesInPrefab[i].isReadable == true)
                {
                    var meshInstance = Instantiate(meshesInPrefab[i]);
                    meshInstance.name = meshesInPrefab[i].name;
                    meshInstances.Add(meshInstance);
                }
                else
                {
                    //Workaround when the mesh is not readable (Unity Bug)
                    ModelImporter modelImporter = AssetImporter.GetAtPath(meshPath) as ModelImporter;

                    if (modelImporter != null)
                    {
                        modelImporter.isReadable = true;
                        modelImporter.SaveAndReimport();
                    }

                    if (meshPath.EndsWith(".asset"))
                    {
                        string filePath = Path.Combine(Directory.GetCurrentDirectory(), meshPath);
                        filePath = filePath.Replace("/", "\\");
                        string fileText = File.ReadAllText(filePath);
                        fileText = fileText.Replace("m_IsReadable: 0", "m_IsReadable: 1");
                        File.WriteAllText(filePath, fileText);
                        AssetDatabase.Refresh();
                    }

                    var meshInstance = Instantiate(meshesInPrefab[i]);
                    meshInstance.name = meshesInPrefab[i].name;
                    meshInstances.Add(meshInstance);
                }
            }
            else
            {
                meshInstances.Add(null);
            }
        }
    }

    bool IsValidGameObject(GameObject gameObject)
    {
        bool valid = true;

        if (gameObject.activeInHierarchy == false)
        {
            valid = false;
        }

        return valid;
    }

    void GetMeshFiltersInPrefab()
    {
        meshFiltersInPrefab = new List<MeshFilter>();

        for (int i = 0; i < gameObjectsInPrefab.Count; i++)
        {
            if (IsValidGameObject(gameObjectsInPrefab[i]) && gameObjectsInPrefab[i].GetComponent<MeshFilter>() != null)
            {
                meshFiltersInPrefab.Add(gameObjectsInPrefab[i].GetComponent<MeshFilter>());
            }
            else
            {
                meshFiltersInPrefab.Add(null);
            }
        }
    }


    void OnWizardUpdate()
    {
        helpString = "Select grass mesh to process, gen pivot and mask data";
        isValid = (prefabObject != null);
    }

    void OnWizardCreate()
    {
        GetGameObjectsInPrefab();
        GetMeshFiltersInPrefab();
        GetMeshesInPrefab();
        CreateMeshInstances();

        for(int i=0;i< meshInstances.Count; i++)
        {
            Mesh mesh = meshInstances[i];
            if(mesh != null)
            {
                ConvertMeshVegetation(mesh);
                break;
            }
        }
    }

    [MenuItem("GameObject/Process Grass Mesh")]
    static void RenderCubemap()
    {
        ScriptableWizard.DisplayWizard<grass_mesh_process>(
            "Process Grass Mesh", "Process");
    }
}

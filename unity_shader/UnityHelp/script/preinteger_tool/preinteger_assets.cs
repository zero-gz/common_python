using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

[CreateAssetMenu]
public class preinteger_assets : ScriptableObject
{
    public Color factor1 = new Color(0.064f, 0.233f, 0.455f, 0.649f);
    public Color factor2 = new Color(0.0484f, 0.100f, 0.336f, 0.344f);
    public Color factor3 = new Color(0.1870f, 0.118f, 0.198f, 0.000f);
    public Color factor4 = new Color(0.5670f, 0.113f, 0.007f, 0.007f);
    public Color factor5 = new Color(1.9900f, 0.358f, 0.004f, 0.000f);
    public Color factor6 = new Color(7.4100f, 0.078f, 0.000f, 0.000f);

    public float x_size = 4.0f;
    public float y_size = 1.6f;

    public int tex_size = 256;
    public string tex_path = "Assets/Resources/";

    // Use this for initialization
    void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}

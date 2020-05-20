using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

[CreateAssetMenu]
public class preinteger_assets : ScriptableObject
{
    public Vector4 factor1 = new Vector4(0.064f, 0.233f, 0.455f, 0.649f);
    public Vector4 factor2 = new Vector4(0.0484f, 0.100f, 0.336f, 0.344f);
    public Vector4 factor3 = new Vector4(0.1870f, 0.118f, 0.198f, 0.000f);
    public Vector4 factor4 = new Vector4(0.5670f, 0.113f, 0.007f, 0.007f);
    public Vector4 factor5 = new Vector4(1.9900f, 0.358f, 0.004f, 0.000f);
    public Vector4 factor6 = new Vector4(7.4100f, 0.078f, 0.000f, 0.000f);

    public float value = 0.2f;

    // Use this for initialization
    void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}

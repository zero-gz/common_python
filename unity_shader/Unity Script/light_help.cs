using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class light_help : MonoBehaviour {
    public GameObject target;
    public Vector2 _center;
    public Color _light_color;
    public float _light_radius;
    public float _falloff;
    public float _light_strength;
    public Vector3 _light_dir;
    // Use this for initialization
    void Start () {
        /*
        _center = new Vector2(0.5f, 0.5f);
        _light_color = new Color(1.0f, 1.0f, 1.0f);
        _light_radius = 1.0f;
        _falloff = 1.0f;
        _light_strength = 1.0f;*/
	}

    void set_target_mtl()
    {
        if (!target) return;
        Material mtl = target.GetComponent<Renderer>().sharedMaterial;

        mtl.SetVector("_center", new Vector4(_center.x, _center.y, 0.0f, 0.0f));
        mtl.SetVector("_light_color", new Vector4(_light_color.r, _light_color.g, _light_color.b, _light_color.a));
        mtl.SetFloat("_light_radius", _light_radius);
        mtl.SetFloat("_falloff", _falloff);
        mtl.SetFloat("_light_strength", _light_strength);
        mtl.SetVector("_light_dir", this.transform.forward);

        _light_dir = this.transform.forward;

        //Debug.Log("------------------------------------------------");
        //Debug.Log(this.transform.forward);
    }
	
	// Update is called once per frame
	void Update () {
        set_target_mtl();
	}
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class camera_render : MonoBehaviour {
    //直接挂在某个camera上
    public Vector2 tex_size = new Vector2(512, 512);
    private RenderTexture rt;
    private Camera m_cam;
    void init_camera_data()
    {
        rt = new RenderTexture((int)(tex_size.x), (int)(tex_size.y), 0);
        m_cam = gameObject.GetComponent<Camera>();
        m_cam.targetTexture = rt;
        m_cam.enabled = false;
    }

    void DebugPanel()
    {
        RawImage img = GameObject.Find("ui/show_rt").GetComponent<RawImage>();
        img.texture = rt;
    }

	// Use this for initialization
	void Start () {
        init_camera_data();
        DebugPanel();
	}
	
	// Update is called once per frame
	void Update () {
		if(Input.GetKeyDown(KeyCode.Space))
        {
            m_cam.Render();
        }
	}
}

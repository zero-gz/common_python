using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class simplex_noise : MonoBehaviour {
    public string save_path;

    public Vector2 frequence;
	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
	}

    public void create_noise()
    {
        Texture2D _texture = new Texture2D(512, 512);
        int seed = System.DateTime.Now.Millisecond;

        for (int x = 0; x < _texture.width; x++)
        {
            for (int y = 0; y < _texture.height; y++)
            {
                float noise = SimplexNoise.SeamlessNoise(((float)x) / ((float)_texture.width),
                                    ((float)y) / ((float)_texture.height),
                                    frequence.x, frequence.y, (float)seed);
                Color color = new Color(noise, noise, noise);
                _texture.SetPixel(x, y, color);
            }
        }
        _texture.Apply();

        util.save_texture(save_path, _texture);
        AssetDatabase.Refresh();
    }
}

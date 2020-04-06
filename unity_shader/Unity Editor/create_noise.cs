using UnityEngine;
using UnityEditor;
using NoiseTools;

public class CreateNoiseTool: ScriptableWizard
{
    public float freq = 2.0f;
    public string save_path = "Assets/Resources/simplex_noise.png";
    public enum NOISE_LIST {SIMPLEX_NOISE, PERLIN_NOISE, WORLEY_NOISE};
    public NOISE_LIST noise_type;

    void OnWizardUpdate()
    {
        helpString = "noise texture generator";
        isValid = true;
    }

    void generator_simplex()
    {
        Texture2D _texture = new Texture2D(512, 512);
        int seed = System.DateTime.Now.Millisecond;

        for (int x = 0; x < _texture.width; x++)
        {
            for (int y = 0; y < _texture.height; y++)
            {
                float noise = simplex_noise.SeamlessNoise(((float)x) / ((float)_texture.width),
                                    ((float)y) / ((float)_texture.height),
                                    freq, freq, (float)seed);
                noise = noise * 0.5f + 0.5f;
                Color color = new Color(noise, noise, noise);
                _texture.SetPixel(x, y, color);
            }
        }
        _texture.Apply();

        util.save_texture(save_path, _texture);
        AssetDatabase.Refresh();
    }

    void generator_perlin()
    {
        int seed = System.DateTime.Now.Millisecond;
        PerlinNoise _noise = new PerlinNoise((int)freq, 1, seed);
        Texture2D _texture = new Texture2D(512, 512);

        for (int x = 0; x < _texture.width; x++)
        {
            for (int y = 0; y < _texture.height; y++)
            {
                Vector2 pos = new Vector2((float)x /(float)(_texture.width), (float)y/(float)(_texture.height) );
                float noise = _noise.GetAt(pos);
                Color color = new Color(noise, noise, noise);
                _texture.SetPixel(x, y, color);
            }
        }
        _texture.Apply();

        util.save_texture(save_path, _texture);
        AssetDatabase.Refresh();
    }

    void generator_worley()
    {
        int seed = System.DateTime.Now.Millisecond;
        WorleyNoise _noise = new WorleyNoise((int)freq, 1, seed);
        Texture2D _texture = new Texture2D(512, 512);

        for (int x = 0; x < _texture.width; x++)
        {
            for (int y = 0; y < _texture.height; y++)
            {
                Vector2 pos = new Vector2((float)x / (float)(_texture.width), (float)y / (float)(_texture.height));
                float noise = _noise.GetAt(pos);
                Color color = new Color(noise, noise, noise);
                _texture.SetPixel(x, y, color);
            }
        }
        _texture.Apply();

        util.save_texture(save_path, _texture);
        AssetDatabase.Refresh();
    }

    void OnWizardCreate()
    {
        if(noise_type == NOISE_LIST.SIMPLEX_NOISE)
        {
            generator_simplex();
        }
        else if(noise_type == NOISE_LIST.PERLIN_NOISE)
        {
            generator_perlin();
        }
        else if(noise_type == NOISE_LIST.WORLEY_NOISE)
        {
            generator_worley();
        }
    }

    [MenuItem("Tools/create_simplex_noise")]
    static void create_simplex_noise()
    {
        ScriptableWizard.DisplayWizard<CreateNoiseTool>("噪声贴图生成", "生成noise贴图");
    }
}
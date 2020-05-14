using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class intergerSSSTool : ScriptableWizard
{
    public int tex_size = 256;
    public string res_path = "Assets/Resources/";

    void OnWizardUpdate()
    {
        helpString = "sss preinteger texture generator";
        isValid = true;
    }

    float Gaussian(float v, float r)
    {
        return 1.0f / Mathf.Sqrt(2.0f * Mathf.PI * v) * Mathf.Exp(-(r * r) / (2.0f * v));
        //return 1.0f / (2.0f * Mathf.PI * v) * Mathf.Exp(-(r * r) / (2.0f * v));
    }

    Vector3 Scatter(float r)
    {
        Vector3 factor1 = new Vector3(0.233f, 0.455f, 0.649f);
        Vector3 factor2 = new Vector3(0.100f, 0.336f, 0.344f);
        Vector3 factor3 = new Vector3(0.118f, 0.198f, 0.000f);
        Vector3 factor4 = new Vector3(0.113f, 0.007f, 0.007f);
        Vector3 factor5 = new Vector3(0.358f, 0.004f, 0.000f);
        Vector3 factor6 = new Vector3(0.078f, 0.000f, 0.000f);

        float sq = 1.0f; // 1.414f;
        return Gaussian(0.0064f*sq, r) * factor1 +
            Gaussian(0.0484f*sq, r) * factor2 +
            Gaussian(0.1870f*sq, r) * factor3 +
            Gaussian(0.5670f*sq, r) * factor4 +
            Gaussian(1.9900f*sq, r) * factor5 +
            Gaussian(7.4100f*sq, r) * factor6;
    }

    float GaussianOther(float v, float r)
    {
        return Mathf.Exp(-(r * r)/v);
    }

    Vector3 ScatterWithGaussOther(float r)
    {
        Vector3 factor1 = new Vector3(0.233f, 0.455f, 0.649f);
        Vector3 factor2 = new Vector3(0.100f, 0.336f, 0.344f);
        Vector3 factor3 = new Vector3(0.118f, 0.198f, 0.000f);
        Vector3 factor4 = new Vector3(0.113f, 0.007f, 0.007f);
        Vector3 factor5 = new Vector3(0.358f, 0.004f, 0.000f);
        Vector3 factor6 = new Vector3(0.078f, 0.000f, 0.000f);

        float sq = 1.0f; // 1.414f;
        return GaussianOther(0.0064f * sq, r) * factor1 +
            GaussianOther(0.0484f * sq, r) * factor2 +
            GaussianOther(0.1870f * sq, r) * factor3 +
            GaussianOther(0.5670f * sq, r) * factor4 +
            GaussianOther(1.9900f * sq, r) * factor5 +
            GaussianOther(7.4100f * sq, r) * factor6;
    }

    Vector3 integrateDiffuseScatteringOnRing(float cosTheta, float skinRadius, float inc)
    {
        //这里为什么不能用上面的公式?
        //float theta = Mathf.Acos(cosTheta);
        float theta = Mathf.PI * (1.0f - cosTheta);
        Vector3 totalWeights = new Vector3(0.0f, 0.0f, 0.0f);
        Vector3 totalLight = new Vector3(0.0f, 0.0f, 0.0f);
        float x = -(Mathf.PI / 2);
        while(x<=(Mathf.PI/2))
        {
            float diffuse = Mathf.Clamp01(Mathf.Cos(theta + x));
            float sampleDist = Mathf.Abs(2.0f * skinRadius * Mathf.Sin(x * 0.5f));
            Vector3 weights = Scatter(sampleDist);
            totalWeights += weights;
            totalLight += diffuse*weights;
            x += inc;
        }
        Vector3 result;
        result.x = totalLight.x / totalWeights.x;
        result.y = totalLight.y / totalWeights.y;
        result.z = totalLight.z / totalWeights.z;
        return result;
    }

    void _gen_tex()
    {
        int width, height;
        width = height = tex_size;
        Texture2D tex = new Texture2D(width, height);
       
        for(int j=0;j<height;j++)
            for (int i = 0; i < width; i++)
            {
                float cosTheta = i*1.0f/width;
                float height_y = j*1.0f/height;
                float skinRadius;
                if (height_y < 0.01f)
                    skinRadius = 100.0f;
                else
                    skinRadius = 1.0f / height_y;

                Vector3 result = integrateDiffuseScatteringOnRing(cosTheta, skinRadius, 0.1f);
                Color col = new Color(result.x, result.y, result.z);
                tex.SetPixel(i, j, col);
            }

        tex.Apply();
        util.save_texture(res_path+"preinteger.png", tex);
    }

    void _gen_diffusion_profile()
    {
        int width, height;
        width = height = tex_size;
        Texture2D tex = new Texture2D(width, height);

        for (int j = 0; j < height; j++)
            for (int i = 0; i < width; i++)
            {
                float cosTheta = i * 1.0f / width;
                float height_y = j * 1.0f / height;

                float radius = 2.0f * Mathf.Sqrt(Mathf.Pow(cosTheta - 0.5f, 2.0f) + Mathf.Pow(height_y - 0.5f, 2.0f));

                Vector3 result = ScatterWithGaussOther(radius);
                Color col = new Color(result.x, result.y, result.z);
                tex.SetPixel(i, j, col);
            }

        tex.Apply();
        util.save_texture(res_path+"preinteger_diffusion_profile.png", tex);
    }

    void OnWizardCreate()
    {
        _gen_tex();
    }

    private void OnWizardOtherButton()
    {
        _gen_diffusion_profile();
    }

    [MenuItem("Tools/create_integer_tex")]
    static void create_integer_tex()
    {
        ScriptableWizard.DisplayWizard<intergerSSSTool>("皮肤SSS图片生成", "生成preinteger贴图", "生成diffusion profile贴图");
    }
}

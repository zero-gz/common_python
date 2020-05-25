using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class intergerSSSTool : ScriptableWizard
{
    public int tex_size = 256;
    public string res_path = "Assets/Resources/";
    public Vector4 factor1 = new Vector4(0.064f, 0.233f, 0.455f, 0.649f);
    public Vector4 factor2 = new Vector4(0.0484f, 0.100f, 0.336f, 0.344f);
    public Vector4 factor3 = new Vector4(0.1870f, 0.118f, 0.198f, 0.000f);
    public Vector4 factor4 = new Vector4(0.5670f, 0.113f, 0.007f, 0.007f);
    public Vector4 factor5 = new Vector4(1.9900f, 0.358f, 0.004f, 0.000f);
    public Vector4 factor6 = new Vector4(7.4100f, 0.078f, 0.000f, 0.000f);

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
        float sq = 1.414f;
        
        return Gaussian(factor1.x*sq, r) * new Vector3(factor1.y, factor1.z, factor1.w) +
            Gaussian(factor2.x*sq, r) * new Vector3(factor2.y, factor2.z, factor2.w) +
            Gaussian(factor3.x*sq, r) * new Vector3(factor3.y, factor3.z, factor3.w) +
            Gaussian(factor4.x*sq, r) * new Vector3(factor4.y, factor4.z, factor4.w) +
            Gaussian(factor5.x*sq, r) * new Vector3(factor5.y, factor5.z, factor5.w) +
            Gaussian(factor6.x*sq, r) * new Vector3(factor6.y, factor6.z, factor6.w);
    }

    float GaussianOther(float v, float r)
    {
        return Mathf.Exp(-(r * r)/v);
    }

    Vector3 ScatterWithGaussOther(float r)
    {
        float sq = 1.0f;
        return GaussianOther(factor1.x * sq, r) * new Vector3(factor1.y, factor1.z, factor1.w) +
             GaussianOther(factor2.x * sq, r) * new Vector3(factor2.y, factor2.z, factor2.w) +
             GaussianOther(factor3.x * sq, r) * new Vector3(factor3.y, factor3.z, factor3.w) +
             GaussianOther(factor4.x * sq, r) * new Vector3(factor4.y, factor4.z, factor4.w) +
             GaussianOther(factor5.x * sq, r) * new Vector3(factor5.y, factor5.z, factor5.w) +
             GaussianOther(factor6.x * sq, r) * new Vector3(factor6.y, factor6.z, factor6.w);
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

                Vector3 result = Scatter(radius); //ScatterWithGaussOther(radius);
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

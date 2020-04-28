using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class paint_rt : MonoBehaviour
{
    public RenderTexture draw_rt;
    private Vector2 last_point;

    public int rt_size = 512;
    public float _brushSize = 60.0f;
    public Texture2D _default_brush_tex;
    private Material _paint_mtl;
    private Material _clear_mtl;
    private float _brushLerpSize;
    public Color _defaultColor = new Color(0.0f, 0.0f, 0.0f);
    public float _brushStength = 1.0f;
    // Use this for initialization
    void Start()
    {
        draw_rt = new RenderTexture(rt_size, rt_size, 0, RenderTextureFormat.ARGB32);
        InitData();
    }

    public virtual void InitData()
    {
        _brushLerpSize = (_default_brush_tex.width + _default_brush_tex.height) / 2.0f / _brushSize;
        last_point = Vector2.zero;

        if (_paint_mtl == null)
        {
            UpdateBrushMaterial();
        }
        if (_clear_mtl == null)
            _clear_mtl = new Material(Shader.Find("my_shader/ClearBrush"));

        Graphics.Blit(null, draw_rt, _clear_mtl);
    }

    //更新笔刷材质
    private void UpdateBrushMaterial()
    {
        _paint_mtl = new Material(Shader.Find("my_shader/PaintBrush"));
        _paint_mtl.SetTexture("_BrushTex", _default_brush_tex);
        _paint_mtl.SetColor("_color", _defaultColor);
        _paint_mtl.SetFloat("_paint_size", _brushSize);
        _paint_mtl.SetFloat("_paint_stength", _brushStength);
    }

    //插点
    private void LerpPaint(Vector2 point)
    {
        Paint(point);

        if (last_point == Vector2.zero)
        {
            last_point = point;
            return;
        }

        float dis = Vector2.Distance(point, last_point);
        if (dis > _brushLerpSize)
        {
            Vector2 dir = (point - last_point).normalized;
            int num = (int)(dis / _brushLerpSize);
            for (int i = 0; i < num; i++)
            {
                Vector2 newPoint = last_point + dir * (i + 1) * _brushLerpSize;
                Paint(newPoint);
            }
        }
        last_point = point;
    }

    //画点
    private void Paint(Vector2 point)
    {
        if (point.x < 0 || point.x > Screen.width || point.y < 0 || point.y > Screen.height)
            return;

        Vector2 uv = new Vector2(point.x / (float)Screen.width,
            point.y / (float)Screen.height);
        //Debug.Log(string.Format("get point data: {0} {1}\t {2} {3}", point.x, point.y, uv.x, uv.y));
        _paint_mtl.SetVector("_UV", uv);
        Graphics.Blit(draw_rt, draw_rt, _paint_mtl);
    }

    private float Remap(float value, float startValue, float enValue)
    {
        float returnValue = (value - 1.0f) / (100.0f - 1.0f);
        returnValue = (enValue - startValue) * returnValue + startValue;
        return returnValue;
    }

    public virtual void after_draw_texture()
    {
        //Debug Code
       //RawImage img = GameObject.Find("ui/show_rt").GetComponent<RawImage>();
       //img.texture = draw_rt;
         
        // paint rendertexture is ok, try to do something
        Debug.Log("call after draw texture, the paint rt is ok!");
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetMouseButton(0))
        {
            LerpPaint(Input.mousePosition);
            after_draw_texture();
        }

        if (Input.GetMouseButtonUp(0))
            last_point = Vector2.zero;
    }
}

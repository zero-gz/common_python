using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class camera_paint_demo : paint_util
{
    public override void after_draw_texture()
    {
        // paint rendertexture is ok, try to do something
        GameObject obj = GameObject.Find("paint_plane");
        Material mtl = obj.GetComponent<Renderer>().material;
        mtl.SetTexture("_MainTex", paint);
    }
}

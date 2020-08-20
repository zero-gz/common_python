// Copyright (c) <2015> <Playdead>
// This file is subject to the MIT License as seen in the root of this folder structure (LICENSE.TXT)
// AUTHOR: Lasse Jon Fuglsang Pedersen <lasse@playdead.com>

#if UNITY_5_5_OR_NEWER
#define SUPPORT_STEREO
#endif

using UnityEngine;

[ExecuteInEditMode]
[RequireComponent(typeof(Camera))]
public class test_frust_jitter : MonoBehaviour
{
    #region Static point data
    private static float[] points_Halton_2_3_x16 = new float[16 * 2];
    #endregion

    #region Static point data, static initialization
    private static void TransformPattern(float[] seq, float theta, float scale)
    {
        float cs = Mathf.Cos(theta);
        float sn = Mathf.Sin(theta);
        for (int i = 0, j = 1, n = seq.Length; i != n; i += 2, j += 2)
        {
            float x = scale * seq[i];
            float y = scale * seq[j];
            seq[i] = x * cs - y * sn;
            seq[j] = x * sn + y * cs;
        }
    }

    // http://en.wikipedia.org/wiki/Halton_sequence
    private static float HaltonSeq(int prime, int index = 1/* NOT! zero-based */)
    {
        float r = 0.0f;
        float f = 1.0f;
        int i = index;
        while (i > 0)
        {
            f /= prime;
            r += f * (i % prime);
            i = (int)Mathf.Floor(i / (float)prime);
        }
        return r;
    }

    private static void InitializeHalton_2_3(float[] seq)
    {
        for (int i = 0, n = seq.Length / 2; i != n; i++)
        {
            float u = HaltonSeq(2, i + 1) - 0.5f;
            float v = HaltonSeq(3, i + 1) - 0.5f;
            seq[2 * i + 0] = u;
            seq[2 * i + 1] = v;
        }
    }

    static test_frust_jitter()
    {
        // points_Halton_2_3_xN
        InitializeHalton_2_3(points_Halton_2_3_x16);
    }
    #endregion

    #region Static point data accessors
    public enum Pattern
    {
        Halton_2_3_X16,
    };

    private static float[] AccessPointData(Pattern pattern)
    {
        switch (pattern)
        {
            case Pattern.Halton_2_3_X16:
                return points_Halton_2_3_x16;
            default:
                Debug.LogError("missing point distribution");
                return points_Halton_2_3_x16;
        }
    }

    public static int AccessLength(Pattern pattern)
    {
        return AccessPointData(pattern).Length / 2;
    }

    public Vector2 Sample(Pattern pattern, int index)
    {
        float[] points = AccessPointData(pattern);
        int n = points.Length / 2;
        int i = index % n;

        float x = patternScale * points[2 * i + 0];
        float y = patternScale * points[2 * i + 1];

        return new Vector2(x, y).Rotate(Vector2.right.SignedAngle(focalMotionDir));
    }
    #endregion

    private Camera _camera;

    private Vector3 focalMotionPos = Vector3.zero;
    private Vector3 focalMotionDir = Vector3.right;

    public Pattern pattern = Pattern.Halton_2_3_X16;
    public float patternScale = 1.0f;

    public Vector4 activeSample = Vector4.zero;// xy = current sample, zw = previous sample
    public int activeIndex = -2;

    void Reset()
    {
        _camera = GetComponent<Camera>();
    }

    void Clear()
    {
        _camera.ResetProjectionMatrix();

        activeSample = Vector4.zero;
        activeIndex = -2;
    }

    void Awake()
    {
        Reset();
        Clear();

        _camera.depthTextureMode = DepthTextureMode.Depth | DepthTextureMode.MotionVectors;
    }

    void OnPreCull()
    {
        // update motion dir
        {
            Vector3 oldWorld = focalMotionPos;
            Vector3 newWorld = _camera.transform.TransformVector(_camera.nearClipPlane * Vector3.forward);

            Vector3 oldPoint = (_camera.worldToCameraMatrix * oldWorld);
            Vector3 newPoint = (_camera.worldToCameraMatrix * newWorld);
            Vector3 newDelta = (newPoint - oldPoint).WithZ(0.0f);

            var mag = newDelta.magnitude;
            if (mag != 0.0f)
            {
                var dir = newDelta / mag;// yes, apparently this is necessary instead of newDelta.normalized... because facepalm
                if (dir.sqrMagnitude != 0.0f)
                {
                    focalMotionPos = newWorld;
                    focalMotionDir = Vector3.Slerp(focalMotionDir, dir, 0.2f);
                    //Debug.Log("CHANGE focalMotionDir " + focalMotionDir.ToString("G4") + " delta was " + newDelta.ToString("G4") + " delta.mag " + newDelta.magnitude);
                }
            }
        }

        // update jitter
        {
            if (activeIndex == -2)
            {
                activeSample = Vector4.zero;
                activeIndex += 1;

                _camera.projectionMatrix = _camera.GetProjectionMatrix();
            }
            else
            {
                activeIndex += 1;
                activeIndex %= AccessLength(pattern);

                Vector2 sample = Sample(pattern, activeIndex);
                activeSample.z = activeSample.x;
                activeSample.w = activeSample.y;
                activeSample.x = sample.x;
                activeSample.y = sample.y;

                _camera.projectionMatrix = _camera.GetProjectionMatrix(sample.x, sample.y);
                Debug.Log(string.Format("get camera projection matrix: %s", _camera.projectionMatrix ));
            }
        }
    }

    void OnDisable()
    {
        Clear();
    }
}

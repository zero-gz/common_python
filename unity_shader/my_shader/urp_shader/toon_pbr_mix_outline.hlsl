#ifndef SIMPLE_TOON_SHADING_OUTLINE_INCLUDED
#define SIMPLE_TOON_SHADING_OUTLINE_INCLUDED

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Lighting.hlsl"
#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"

CBUFFER_START(UnityPerMaterial)
half _OutlineWidth;
half4 _OutLineColor;
CBUFFER_END

struct Attributes
{
    float4 positionOS : POSITION;
    float3 normalOS : NORMAL;
    float4 tangentOS : TANGENT;
    float2 texcoord : TEXCOORD0;
};

struct Varyings
{
    float4 positionHCS : SV_POSITION;
};

///////////////////////////////////////////////////////////////////////////////
//                  Vertex and Fragment functions                            //
///////////////////////////////////////////////////////////////////////////////

// Used in Standard (Simple Lighting) shader
Varyings LitPassVertexSimple(Attributes input)
{
    Varyings output = (Varyings) 0;
    float4 pos = TransformObjectToHClip(input.positionOS.xyz);
    
    float3 viewNormal = mul((float3x3) UNITY_MATRIX_IT_MV, input.normalOS.xyz);

	// normalize(TransformViewToProjection(viewNormal.xyz))�������Ѿ��õ�NDC�ռ��normal�ˣ�������Ϊ��Ҫ��
	// pos.xy�����죬���������/pos.w�ģ����������ȳ���pos.w
    float2 offset = normalize(mul((float3x3) UNITY_MATRIX_P, viewNormal.xyz)).xy; //�����߱任��NDC�ռ�
    float4 nearUpperRight = mul(unity_CameraInvProjection, float4(1, 1, UNITY_NEAR_CLIP_VALUE, _ProjectionParams.y)); //�����ü������Ͻǵ�λ�õĶ���任���۲�ռ�
    float aspect = abs(nearUpperRight.y / nearUpperRight.x); //�����Ļ��߱�
    offset.x *= aspect;

    pos.xy += 0.01 * _OutlineWidth * offset * pos.w;
    pos.z -= 0.01;
    output.positionHCS = pos;
    
    return output;
}

// Used for StandardSimpleLighting shader
half4 LitPassFragmentSimple(Varyings i) : SV_Target
{
    return _OutLineColor;
};

#endif

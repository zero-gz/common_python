#ifndef SIMPLE_TOON_SHADING_INPUT_INCLUDED
#define SIMPLE_TOON_SHADING_INPUT_INCLUDED

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
TEXTURE2D(_MainTex);    SAMPLER(sampler_MainTex);

TEXTURE2D(_albedo_tex);    SAMPLER(sampler_albedo_tex);
TEXTURE2D(_normal_tex);    SAMPLER(sampler_normal_tex);
TEXTURE2D(_mix_tex);    SAMPLER(sampler_mix_tex);
TEXTURE2D(_emissive_tex);    SAMPLER(sampler_emissive_tex);
TEXTURE2D(_preinteger_tex);  SAMPLER(sampler_preinteger_tex);

CBUFFER_START(UnityPerMaterial)
float _metallic;
float _roughness;
float4 _emissive;

half3 _MainColor;
half3 _ShadowColor;
half _ShadowRange;
float _ShadowSmooth;
float _tex_ctrl_threshold;

float _toon_diffuse_intensity;
float _pbr_specular_intensity;
float _gi_intensity;

float4 _RimColor;
float _RimMin;
float _RimMax;
float _RimSmooth;
CBUFFER_END

struct LightingVars
{
    float3 T;
    float3 B;
    float3 N;
    float3 V;
    float3 L;
    float3 H;

    float3 albedo;
    float3 diffuse_color;
    float3 f0;
    float roughness;
    float metallic;
    float opacity;
    float3 light_color;

#if defined(LIGHTMAP_ON)
        float4 lightmap_uv;
#endif

    float3 world_pos;
    float occlusion; //遮蔽 全局光，包括indrect diffuse和specular
    float shadow; // 遮蔽实时灯光
};

struct MaterialVars
{
    float3 albedo;
    float3 normal;
    float roughness;
    float metallic;
    float3 emissive;
    float opacity;
    float occlusion;
};

struct LightingResult
{
    float3 lighting_diffuse;
    float3 lighting_specular;
    float3 lighting_scatter;
};

#endif

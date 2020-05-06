#ifndef __COMMON_VAR__
#define __COMMON_VAR__

struct appdata
{
	float4 vertex : POSITION;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float4 tangent : TANGENT;
#ifdef LIGHTMAP_ON
	float2 uv1 : TEXCOORD1;
#endif

#ifdef DYNAMICLIGHTMAP_ON
	float2 uv2 : TEXCOORD2;
#endif
};

struct v2f
{
	float2 uv : TEXCOORD0;
	float4 pos : SV_POSITION;
	float3 world_pos: TEXCOORD1;
	float3 world_normal: TEXCOORD2;
	float3 world_tangent: TEXCOORD3;
	float3 world_binnormal: TEXCOORD4;
#if defined(LIGHTMAP_ON) || defined(DYNAMICLIGHTMAP_ON)
	float4 lightmap_uv: TEXCOORD5;
#endif

	SHADOW_COORDS(6)
};

struct LightingVars {
	float3 T;
	float3 B;
	float3 N;
	float3 V;
	float3 L;
	float3 H;

	float3 diffuse_color;
	float3 f0;
	float roughness;
	float metallic;

	float3 light_color;

#if defined(LIGHTMAP_ON) || defined(DYNAMICLIGHTMAP_ON)
	float4 lightmap_uv;
#endif

	float3 world_pos;
	float occlusion; //遮蔽 全局光，包括indrect diffuse和specular
	float shadow; // 遮蔽实时灯光
	float3 emissive;
};

struct MaterialVars {
	float3 albedo;
	float3 normal;
	float roughness;
	float metallic;
	float3 emissive;
	float opacity;
	float occlusion;
};

struct LightingResult {
	float3 lighting_diffuse;
	float3 lighting_specular;
};

sampler2D _albedo_tex;
sampler2D _normal_tex;
sampler2D _mix_tex;

float _roughness;
float _metallic;
float3 _emissive;

#endif
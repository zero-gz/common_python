#ifndef EFFECTS_INCLUDE
#define EFFECTS_INCLUDE

#include "common.cginc"

//  TA技法
// effect color_tint
/*
sampler2D _id_tex;
uniform float4 _color_tint1;
uniform float4 _color_tint2;
uniform float4 _color_tint3;

void effect_color_tint(v2f i, inout MaterialVars mtl, inout LightingVars data)
{
	float4 mask_value = tex2D(_id_tex, i.uv);
	float gray_value = dot(mtl.albedo, float3(0.299f, 0.587f, 0.114f));
	float3 tint1_result = lerp(mtl.albedo, _color_tint1.rgb*_color_tint1.a*gray_value, float3(mask_value.r, mask_value.r, mask_value.r));
	float3 tint2_result = lerp(tint1_result, _color_tint2.rgb*_color_tint2.a*gray_value, float3(mask_value.g, mask_value.g, mask_value.g));
	float3 tint3_result = lerp(tint2_result, _color_tint3.rgb*_color_tint3.a*gray_value, float3(mask_value.b, mask_value.b, mask_value.b));

	mtl.albedo = tint3_result;
}
*/


// effect emissive
/*
sampler2D _emissive_mask_map;

void effect_emissive(v2f i, inout MaterialVars mtl, inout LightingVars data)
{
	float4 mask_value = tex2D(_emissive_mask_map, i.uv);
	mtl.emissive = _emissive*mask_value.r;
}
*/


// effect fresnel
/*
uniform float4 _fresnel_color;
uniform float _fresnel_scale;
uniform float _fresnel_bias;
uniform float _fresnel_power;

// schlick近似公式
float fresnel_standard_node(float fresnel_scale, float3 V, float3 N)
{
	return fresnel_scale + (1.0 - fresnel_scale)*pow((1.0 - dot(V, N)), 5.0);
}

// Empricial近似公式， 这个控制变量更多，可以把效果做的更柔和一点
float fresnel_simulate_node(float fresnel_bias, float fresnel_scale, float fresnel_power, float3 V, float3 N)
{
	return max(0.0, min(1.0, fresnel_bias + fresnel_scale * pow((1.0 - dot(V, N)), fresnel_power)) );
}

// effect outline color
void effect_fresnel_color(v2f i, inout MaterialVars mtl, inout LightingVars data)
{
	//float fresnel_factor = fresnel_standard_node(_fresnel_scale, data.V, data.N);
	float fresnel_factor = fresnel_simulate_node(_fresnel_bias, _fresnel_scale, _fresnel_power, data.V, data.N);
	mtl.albedo = lerp(mtl.albedo, _fresnel_color.rgb, float3(fresnel_factor, fresnel_factor, fresnel_factor) );
}
*/
#endif
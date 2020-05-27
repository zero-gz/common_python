#ifndef BRDF_INCLUDE
#define BRDF_INCLUDE

/*
    to add a new direct lighting model, you need to fill the struct LightingResult, diffuse and specular lighting.
    LightingVars containes the most vars you need, you can find the definition in common.cginc;
*/

#include "common.cginc"

//  ################   blinnphone lighting

LightingResult direct_blinnphone_lighting(LightingVars data)
{
    LightingResult result;
    result.lighting_diffuse = data.light_color*data.diffuse_color*max(dot(data.N, data.L), 0.0);
    result.lighting_specular = data.light_color*data.f0*pow(max(dot(data.H, data.N), 0.0), 32) ;
    return result;
}

//   ############### cook-torrance pbr lighting

float3 Diffuse_Lambert(float3 DiffuseColor)
{
    return DiffuseColor * (1 / PI);
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX( float a2, float NoH )
{
    float d = ( NoH * a2 - NoH ) * NoH + 1;	// 2 mad
    return a2 / ( PI*d*d );					// 4 mul, 1 rcp
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox( float a2, float NoV, float NoL )
{
    float a = sqrt(a2);
    float Vis_SmithV = NoL * ( NoV * ( 1 - a ) + a );
    float Vis_SmithL = NoV * ( NoL * ( 1 - a ) + a );
    return 0.5 * rcp( Vis_SmithV + Vis_SmithL );
}

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float3 F_Schlick( float3 f0, float VoH )
{
    float Fc = Pow5( 1 - VoH );
    return Fc + f0*(1 - Fc);
}			

float3 SpecularGGX(LightingVars data)
{
    float Roughness = data.roughness;
    float NoH = max(saturate(dot(data.N, data.H)), CHAOS);
    float NoL = max(saturate(dot(data.N, data.L)), CHAOS);
    float NoV = max(saturate(dot(data.N, data.V)), CHAOS);
    float VoH = max(saturate(dot(data.V, data.H)), CHAOS);

    // mtl中存放的是 感知线性粗糙度（为了方便美术调整，所以值为实际值的sqrt)				
    float use_roughness = max(Pow2(Roughness), 0.002);
    float a2 = Pow2(use_roughness);
    //float Energy = EnergyNormalization( a2, Context.VoH, AreaLight );
    float Energy = 1.0;
    
    // Generalized microfacet specular
    float D = D_GGX( a2, NoH ) * Energy;
    float Vis = Vis_SmithJointApprox( a2, NoV, NoL);
    float3 F = F_Schlick(data.f0, VoH);

    return (D * Vis) * F;
}

// Specular term
// HACK: theoretically we should divide diffuseTerm by Pi and not multiply specularTerm!
// BUT 1) that will make shader look significantly darker than Legacy ones
// and 2) on engine side "Non-important" lights have to be divided by Pi too in cases when they are injected into ambient SH
LightingResult isotropy_lighting(LightingVars data)
{
    LightingResult result;

    float NoL = max(dot(data.N, data.L), 0.0);
	// unity的问题，乘以了pi
	result.lighting_diffuse = (data.light_color*NoL) * Diffuse_Lambert(data.diffuse_color)*PI;
    result.lighting_specular = (data.light_color*NoL) * SpecularGGX(data)*PI;
	result.lighting_scatter = float3(0.0, 0.0, 0.0);
    return result;
}

LightingResult subsurface_lighting(LightingVars data)
{
	LightingResult result;

	float NoL = max(dot(data.N, data.L), 0.0);
	result.lighting_diffuse = (data.light_color*NoL) * Diffuse_Lambert(data.diffuse_color)*PI;
	result.lighting_specular = (data.light_color*NoL) * SpecularGGX(data)*PI;

	//加上反向的透射部分
	float trans_dot = pow(saturate(dot(data.V, -data.L)), _sss_power)*_sss_strength*data.opacity;
	result.lighting_scatter = trans_dot * data.sss_color;
	
	/*
	data.opacity = 1.0-_sss_strength;
	float in_scatter = pow(saturate(dot(data.V, -data.L) ), 12.0) * lerp(3.0, 0.1, data.opacity);
	float normal_contribution = dot(data.N, data.H)*data.opacity + 1.0 - data.opacity;
	float back_scatter = normal_contribution / (2.0*PI);
	result.lighting_scatter = data.sss_color*lerp(back_scatter, 1.0, in_scatter);
	*/	
	return result;
}

LightingResult skin_lighting(LightingVars data)
{
	LightingResult result;

	float curvature = saturate( length(fwidth(data.N)) / length(fwidth(data.world_pos)));

	float NoL = max(dot(data.N, data.L), 0.0);
	float2 preinteger_uv;
	preinteger_uv.x = dot(data.N, data.L)*0.5 + 0.5;
	preinteger_uv.y = saturate( curvature * dot(data.light_color, float3(0.22, 0.707, 0.071)) );
	float3 brdf = tex2D(_preinteger_tex, preinteger_uv).rgb;

	result.lighting_diffuse = (data.light_color*lerp(NoL, brdf, data.thickness)) * Diffuse_Lambert(data.diffuse_color)*PI;
	result.lighting_specular = (data.light_color*NoL) * SpecularGGX(data)*PI;

	//加上反向的透射部分，其实 前面nol允许负数取值，就说明了暗部不是一个纯黑，而是一个有微弱红色色彩偏向的值
	//float trans_dot = pow(saturate(dot(data.V, -data.L)), _sss_power)*_sss_strength*data.thickness;
	result.lighting_scatter = float3(0,0,0); // trans_dot * data.sss_color;
	return result;
}

float3 SpecularAnisotropic(LightingVars data)
{
	/*
	//某一种各项异性，带物理性的公式 这里的负号是为什么？？ 神一般的操作啊
	float LoT = dot(-data.L, data.T);
	float VoT = dot(data.V, data.T);
	float shiniess = 1.0f - data.roughness;
	float factor = pow(sqrt(1.0 - LoT * LoT)*sqrt(1.0 - VoT * VoT) + LoT * VoT, shiniess*256.0f)*_anisotropy_intensity;
	return max(0.0, factor)*data.f0;
	*/

	//kajiya的高光公式
	/*
	float shiniess = 1.0f - data.roughness;
	float ToH = dot(data.T, data.H);
	float factor = pow(sqrt(1.0 - ToH * ToH), shiniess*256.0f)*_anisotropy_intensity;
	return max(0.0, factor)*data.f0;
	*/

	
	float at = max(data.roughness*(1.0 + _anisotropy), 0.001f);
	float ab = max(data.roughness*(1.0 - _anisotropy), 0.001f);

	float NoH = max(saturate(dot(data.N, data.H)), CHAOS);
	float NoL = max(saturate(dot(data.N, data.L)), CHAOS);
	float NoV = max(saturate(dot(data.N, data.V)), CHAOS);
	float VoH = max(saturate(dot(data.V, data.H)), CHAOS);

	float ToH = dot(data.T, data.H);
	float BoH = dot(data.B, data.H);

	float ToV = dot(data.T, data.V);
	float BoV = dot(data.B, data.V);
	float ToL = dot(data.T, data.L);
	float BoL = dot(data.B, data.L);

	// D项
	float a2 = at * ab;
	float3 v = float3(ab*ToH, at*BoH, a2*NoH);
	float v2 = dot(v, v);
	float w2 = a2 / v2;
	float D = a2 * w2*w2*(1.0 / PI);

	// V项
	float lambdaV = NoL * length(float3(at * ToV, ab * BoV, NoV));
	float lambdaL = NoV * length(float3(at * ToL, ab * BoL, NoL));
	float Vis = 0.5 / (lambdaV + lambdaL);

	// F项
	float3 F = F_Schlick(data.f0, VoH);

	return (D * Vis) * F * _anisotropy_intensity;
	
}

LightingResult hair_lighting(LightingVars data)
{
	LightingResult result;

	float NoL = max(dot(data.N, data.L), 0.0);
	result.lighting_diffuse = (data.light_color*NoL) * Diffuse_Lambert(data.diffuse_color)*PI;

	// T需要进行jitter
	float jitter = tex2D(_hair_jitter, data.base_vars.uv0).r;
	data.T = data.T + data.N*jitter*_jitter_scale;

	//float2 new_T = tex2D(_hair_tangent, data.base_vars.uv0).rg;
	//data.T = data.T*new_T.x + data.B*new_T.y;

	result.lighting_specular = (data.light_color*NoL) * SpecularAnisotropic(data)*PI;
	result.lighting_scatter = float3(0, 0, 0);
	return result;
}

LightingResult direct_lighting(LightingVars data)
{
	#if _LIGHTING_TYPE_DEFAULT
		return isotropy_lighting(data);
	#elif _LIGHTING_TYPE_SUBSURFACE
		return subsurface_lighting(data);
	#elif _LIGHTING_TYPE_SKIN
		return skin_lighting(data);
	#elif _LIGHTING_TYPE_HAIR
		return hair_lighting(data);
	#else
		return isotropy_lighting(data);
	#endif
}
#endif
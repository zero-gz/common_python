#ifndef TOON_PBR_MIX_GI
#define TOON_PBR_MIX_GI

#include "toon_pbr_mix_input.hlsl"

void init_result(inout LightingResult result)
{
	result.lighting_diffuse = float3(0.0, 0.0, 0.0);
	result.lighting_specular = float3(0.0, 0.0, 0.0);
	result.lighting_scatter = float3(0.0, 0.0, 0.0);
}

float3 ibl_lighting_diffuse(LightingVars data)
{
	return data.diffuse_color * SampleSH(data.N);
}

// Env BRDF Approx
float3 env_approx(LightingVars data)
{
	float NoV = max(saturate(dot(data.N, data.V)), CHAOS);

	float4 C0 = float4(-1.000f, -0.0275f, -0.572f, 0.022f);
	float4 C1 = float4(1.000f, 0.0425f, 1.040f, -0.040f);
	float2 C2 = float2(-1.040f, 1.040f);
	float4 r = C0 * data.roughness + C1;
	float a = min(r.x * r.x, exp2(-9.28f * NoV)) * r.x + r.y;
	float2 ab = C2 * a + r.zw;

	return data.f0*ab.x + float3(ab.y, ab.y, ab.y);
}

float3 ibl_lighting_specular(LightingVars data)
{
	// ibl specular part1
	float mip_roughness = data.roughness * (1.7 - 0.7 * data.roughness);
	float3 reflectVec = reflect(-data.V, data.N);

	half mip = mip_roughness * UNITY_SPECCUBE_LOD_STEPS;
	half4 rgbm = SAMPLE_TEXTURECUBE_LOD(unity_SpecCube0, samplerunity_SpecCube0, reflectVec, mip);

#if !defined(UNITY_USE_NATIVE_HDR)
	float3 iblSpecular = DecodeHDREnvironment(rgbm, unity_SpecCube0_HDR);
#else
	float3 iblSpecular = rgbm.rgb;
#endif

	// ibl specular part2
	float3 brdf_factor = env_approx(data);

	return iblSpecular * brdf_factor;
}

float3 lightmap_lighting_diffuse(LightingVars data)
{
#if defined(LIGHTMAP_ON)
	float2 lightmapUV = data.lightmap_uv;
#else
	float2 lightmapUV = 0;
#endif
	float3 normalWS = data.N;
#ifdef UNITY_LIGHTMAP_FULL_HDR
	bool encodedLightmap = false;
#else
	bool encodedLightmap = true;
#endif

	half4 decodeInstructions = half4(LIGHTMAP_HDR_MULTIPLIER, LIGHTMAP_HDR_EXPONENT, 0.0h, 0.0h);

	// The shader library sample lightmap functions transform the lightmap uv coords to apply bias and scale.
	// However, universal pipeline already transformed those coords in vertex. We pass half4(1, 1, 0, 0) and
	// the compiler will optimize the transform away.
	half4 transformCoords = half4(1, 1, 0, 0);

#ifdef DIRLIGHTMAP_COMBINED
	return SampleDirectionalLightmap(TEXTURE2D_ARGS(unity_Lightmap, samplerunity_Lightmap),
		TEXTURE2D_ARGS(unity_LightmapInd, samplerunity_Lightmap),
		lightmapUV, transformCoords, normalWS, encodedLightmap, decodeInstructions);
#elif defined(LIGHTMAP_ON)
	return SampleSingleLightmap(TEXTURE2D_ARGS(unity_Lightmap, samplerunity_Lightmap), lightmapUV, transformCoords, encodedLightmap, decodeInstructions);
#else
	return half3(0.0, 0.0, 0.0);
#endif
}


LightingResult gi_isotropy_lighting(LightingVars data)
{
	LightingResult result;
	init_result(result);

#ifdef UNITY_SPECCUBE_BOX_PROJECTION
	result.lighting_specular += ibl_lighting_specular(data);
#endif	

#if defined(LIGHTMAP_ON)
	result.lighting_diffuse += lightmap_lighting_diffuse(data);
#else
	result.lighting_diffuse += ibl_lighting_diffuse(data);
#endif

	return result;
}

LightingResult gi_lighting(LightingVars data)
{
	return gi_isotropy_lighting(data);
}

#endif
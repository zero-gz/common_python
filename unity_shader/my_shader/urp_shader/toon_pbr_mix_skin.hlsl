#ifndef TOON_PBR_MIX_SKIN
#define TOON_PBR_MIX_SKIN

#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Lighting.hlsl"
#include "toon_pbr_mix_brdf.hlsl"
#include "toon_pbr_mix_gi.hlsl"

struct Attributes
{
    float4 positionOS : POSITION;
    float3 normalOS : NORMAL;
    float4 tangentOS : TANGENT;
    float2 texcoord : TEXCOORD0;
    float2 lightmapUV : TEXCOORD1;
};

struct Varyings
{
    float4 positionHCS : SV_POSITION;
    float2 uv : TEXCOORD0;
    DECLARE_LIGHTMAP_OR_SH(lightmapUV, vertexSH, 1);

    float3 world_pos : TEXCOORD2;
    float3 world_normal : TEXCOORD3;
float3 world_tangent : TEXCOORD4;
float3 world_binnormal : TEXCOORD5;
#if defined(REQUIRES_VERTEX_SHADOW_COORD_INTERPOLATOR)
    float4 shadowCoord              : TEXCOORD6;
#endif
};

///////////////////////////////////////////////////////////////////////////////
//                  Vertex and Fragment functions                            //
///////////////////////////////////////////////////////////////////////////////

// Used in Standard (Simple Lighting) shader
Varyings LitPassVertexSimple(Attributes input)
{
    Varyings output = (Varyings) 0;
    output.positionHCS = TransformObjectToHClip(input.positionOS.xyz);
    output.uv = input.texcoord;
    
    VertexNormalInputs normalInput = GetVertexNormalInputs(input.normalOS, input.tangentOS);
    output.world_normal = normalInput.normalWS;
    output.world_tangent = normalInput.tangentWS;
    output.world_binnormal = normalInput.bitangentWS;
   
    OUTPUT_LIGHTMAP_UV(input.lightmapUV, unity_LightmapST, output.lightmapUV);
    OUTPUT_SH(normalInput.normalWS, output.vertexSH);
    
    output.world_pos = TransformObjectToWorld(input.positionOS.xyz);
    
    VertexPositionInputs vertexInput = GetVertexPositionInputs(input.positionOS.xyz);
    
#if defined(REQUIRES_VERTEX_SHADOW_COORD_INTERPOLATOR)
    output.shadowCoord = GetShadowCoord(vertexInput);
#endif    
    return output;
}


MaterialVars gen_material_vars(Varyings i)
{
    MaterialVars mtl;
    float4 albedo_color = SAMPLE_TEXTURE2D(_albedo_tex, sampler_albedo_tex, i.uv);
    mtl.albedo = albedo_color.rgb;

    float3 normal_color = SAMPLE_TEXTURE2D(_normal_tex, sampler_normal_tex, i.uv).rgb;
    mtl.normal = normal_color * 2.0 - 1.0;

    mtl.roughness = _roughness; // SAMPLE_TEXTURE2D(_mix_tex, sampler_mix_tex, i.uv).g;
    mtl.metallic = _metallic; // SAMPLE_TEXTURE2D(_mix_tex, sampler_mix_tex, i.uv).r;
    mtl.emissive = _emissive.rgb * SAMPLE_TEXTURE2D(_emissive_tex, sampler_emissive_tex, i.uv).r;
    mtl.opacity = albedo_color.a;
    mtl.occlusion = 1.0;
    return mtl;
}

LightingVars gen_lighting_vars(Varyings i, MaterialVars mtl, Light light_data)
{
    LightingVars data;
    data.T = normalize(i.world_tangent);
    data.B = normalize(i.world_binnormal);
    data.N = normalize(normalize(i.world_tangent) * mtl.normal.x + normalize(i.world_binnormal) * mtl.normal.y + normalize(i.world_normal) * mtl.normal.z);

    data.V = normalize(_WorldSpaceCameraPos.xyz - i.world_pos.xyz);
    data.L = light_data.direction;
    data.H = normalize(data.V + data.L);
    data.albedo = mtl.albedo;
    data.diffuse_color = mtl.albedo * (1.0 - mtl.metallic);
    data.f0 = lerp(float3(0.04, 0.04, 0.04), mtl.albedo, mtl.metallic);
    data.roughness = mtl.roughness;
    data.metallic = mtl.metallic;
    data.opacity = mtl.opacity;

    data.light_color = light_data.color*light_data.distanceAttenuation;

#if defined(LIGHTMAP_ON) || defined(DYNAMICLIGHTMAP_ON)
	data.lightmap_uv = i.lightmap_uv;
#endif

    data.world_pos = i.world_pos;

    data.occlusion = mtl.occlusion;
    data.shadow = light_data.shadowAttenuation;
    return data;
}

// Used for StandardSimpleLighting shader
half4 LitPassFragmentSimple(Varyings i) : SV_Target
{
    float4 final_color = 0;
    MaterialVars mtl = gen_material_vars(i);
    
#if defined(REQUIRES_VERTEX_SHADOW_COORD_INTERPOLATOR) 
    Light light_data = GetMainLight(i.shadowCoord);
#else
    Light light_data = GetMainLight();
#endif
    
    LightingVars data = gen_lighting_vars(i, mtl, light_data);
    LightingResult lighting_result = isotropy_lighting(data);
	LightingResult gi_result = gi_lighting(data);
    
    final_color.rgb = lighting_result.lighting_diffuse + lighting_result.lighting_specular;
	final_color.rgb += gi_result.lighting_diffuse + gi_result.lighting_specular;

    return final_color;
};

#endif

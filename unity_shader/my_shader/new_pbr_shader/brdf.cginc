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

LightingResult direct_isotropy_lighting(LightingVars data)
{
    LightingResult result;

    float NoL = max(dot(data.N, data.L), 0.0);
    result.lighting_diffuse = (data.light_color*NoL) * Diffuse_Lambert(data.diffuse_color);
    result.lighting_specular = (data.light_color*NoL) * SpecularGGX(data);
    return result;
}

#endif
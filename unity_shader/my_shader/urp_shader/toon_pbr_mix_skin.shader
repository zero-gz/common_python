// Shader targeted for low end devices. Single Pass Forward Rendering.
Shader "urp_toon_pbr/skin"
{
	// Keep properties of StandardSpecular shader for upgrade reasons.
	Properties
	{
		[Space(10)]
		[Toggle]ENABLE_DIFFUSE("ENABLE_DIFFUSE", Float) = 1
		[Toggle]ENABLE_PBR_SPECULAR("ENABLE_PBR_SPECULAR", Float) = 1
		[Toggle]ENABLE_GI("ENABLE_GI", Float) = 1
		_toon_diffuse_intensity("toon diffuse intensity", Range(0, 5)) = 1.0
		_pbr_specular_intensity("pbr specular intensity", Range(0, 5)) = 1.0
		_gi_intensity("gi intensity", Range(0, 5)) = 1.0

		[Space(30)]
		_albedo_tex("albedo texture", 2D) = "white" {}
		_MainColor("Main Color", Color) = (1,1,1)
		_ShadowColor("Shadow Color", Color) = (0.7, 0.7, 0.8)
		_ShadowRange("Shadow Range", Range(0, 1)) = 0.5
		_ShadowSmooth("Shadow Smooth", Range(0, 1)) = 0.2
		_tex_ctrl_threshold("ilm texture g channel", Range(0, 1)) = 0.5

		[Space(30)]
		_normal_tex("normal texture", 2D) = "bump"{}
		_metallic("metallic", Range(0, 1)) = 0
		_roughness("roughness", Range(0, 1)) = 0
		_mix_tex("mix texture (R metallic, G roughness)", 2D) = "black" {}

		[Space(20)]
		[HDR]_emissive("Emissive", Color) = (0.0, 0.0, 0.0, 0.0)
		_emissive_tex("Emissive mask texture", 2D) = "white"{}

		[Space(30)]
		[Toggle]ENABLE_RIMLIGHT("ENABLE_RIMLIGHT", Float) = 0
		_RimColor("Rim Color", Color) = (0,0,0,1)
		_RimMin("Rim min", Range(0,1)) = 0
		_RimMax("Rim max", Range(0, 1)) = 1
		_RimSmooth("Rim smooth", Range(0, 1)) = 1

		[Space(30)]
		_OutlineWidth("Outline Width", Range(0.01, 2)) = 0.24
		_OutLineColor("OutLine Color", Color) = (0.5,0.5,0.5,1)

		// Blending state
		[HideInInspector] _Surface("__surface", Float) = 0.0
		[HideInInspector] _Blend("__blend", Float) = 0.0
		[HideInInspector] _AlphaClip("__clip", Float) = 0.0
		[HideInInspector] _SrcBlend("__src", Float) = 1.0
		[HideInInspector] _DstBlend("__dst", Float) = 0.0
		[HideInInspector] _ZWrite("__zw", Float) = 1.0
		[HideInInspector] _Cull("__cull", Float) = 2.0

		// Editmode props
		[HideInInspector] _QueueOffset("Queue offset", Float) = 0.0
	}

		SubShader
			{
				Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalPipeline" "IgnoreProjector" = "True"}
				LOD 300

				Pass
				{
					Name "ForwardLit"
					Tags { "LightMode" = "UniversalForward" }

				// Use same blending / depth states as Standard shader
				Blend[_SrcBlend][_DstBlend]
				ZWrite[_ZWrite]
				Cull[_Cull]

				HLSLPROGRAM
				// Required to compile gles 2.0 with standard srp library
				#pragma prefer_hlslcc gles
				#pragma exclude_renderers d3d11_9x
				#pragma target 5.0

				// -------------------------------------
				// Material Keywords
				#pragma shader_feature ENABLE_DIFFUSE_ON
				#pragma shader_feature ENABLE_PBR_SPECULAR_ON
				#pragma shader_feature ENABLE_GI_ON
				#pragma shader_feature ENABLE_RIMLIGHT_ON	

				// -------------------------------------
				// Universal Pipeline keywords
				#pragma multi_compile _ _MAIN_LIGHT_SHADOWS
				#pragma multi_compile _ _MAIN_LIGHT_SHADOWS_CASCADE
				#pragma multi_compile _ _ADDITIONAL_LIGHTS_VERTEX _ADDITIONAL_LIGHTS
				#pragma multi_compile _ _ADDITIONAL_LIGHT_SHADOWS
				#pragma multi_compile _ _SHADOWS_SOFT
				#pragma multi_compile _ _MIXED_LIGHTING_SUBTRACTIVE

				// -------------------------------------
				// Unity defined keywords
				#pragma multi_compile _ DIRLIGHTMAP_COMBINED
				#pragma multi_compile _ LIGHTMAP_ON
				#pragma multi_compile_fog

				//--------------------------------------
				// GPU Instancing
				#pragma multi_compile_instancing

				#pragma vertex LitPassVertexSimple
				#pragma fragment LitPassFragmentSimple
				#pragma enable_d3d11_debug_symbols

				#include "toon_pbr_mix_input.hlsl"
				#include "toon_pbr_mix_skin.hlsl"
				ENDHLSL
			}

			Pass
			{
				Name "ForwardLitOutline"
				Tags { "LightMode" = "SRPDefaultUnlit" }

				// Use same blending / depth states as Standard shader
				Blend[_SrcBlend][_DstBlend]
				ZWrite[_ZWrite]
				Cull[_Cull]

				HLSLPROGRAM
				// Required to compile gles 2.0 with standard srp library
				#pragma prefer_hlslcc gles
				#pragma exclude_renderers d3d11_9x
				#pragma target 5.0

				// -------------------------------------
				// Material Keywords

				// -------------------------------------
				// Universal Pipeline keywords
				#pragma multi_compile _ _MAIN_LIGHT_SHADOWS
				#pragma multi_compile _ _MAIN_LIGHT_SHADOWS_CASCADE
				#pragma multi_compile _ _ADDITIONAL_LIGHTS_VERTEX _ADDITIONAL_LIGHTS
				#pragma multi_compile _ _ADDITIONAL_LIGHT_SHADOWS
				#pragma multi_compile _ _SHADOWS_SOFT
				#pragma multi_compile _ _MIXED_LIGHTING_SUBTRACTIVE

				// -------------------------------------
				// Unity defined keywords
				#pragma multi_compile _ DIRLIGHTMAP_COMBINED
				#pragma multi_compile _ LIGHTMAP_ON
				#pragma multi_compile_fog

				//--------------------------------------
				// GPU Instancing
				#pragma multi_compile_instancing

				#pragma vertex LitPassVertexSimple
				#pragma fragment LitPassFragmentSimple
				#pragma enable_d3d11_debug_symbols

				#include "toon_pbr_mix_outline.hlsl"
				ENDHLSL
			}

			Pass
			{
				Name "ShadowCaster"
				Tags{"LightMode" = "ShadowCaster"}

				ZWrite On
				ZTest LEqual
				Cull[_Cull]

				HLSLPROGRAM
				// Required to compile gles 2.0 with standard srp library
				#pragma prefer_hlslcc gles
				#pragma exclude_renderers d3d11_9x
				#pragma target 2.0

				// -------------------------------------
				// Material Keywords
				#pragma shader_feature _ALPHATEST_ON
				#pragma shader_feature _GLOSSINESS_FROM_BASE_ALPHA

				//--------------------------------------
				// GPU Instancing
				#pragma multi_compile_instancing

				#pragma vertex ShadowPassVertex
				#pragma fragment ShadowPassFragment

				#include "Packages/com.unity.render-pipelines.universal/Shaders/SimpleLitInput.hlsl"
				#include "Packages/com.unity.render-pipelines.universal/Shaders/ShadowCasterPass.hlsl"
				ENDHLSL
			}

			Pass
			{
				Name "DepthOnly"
				Tags{"LightMode" = "DepthOnly"}

				ZWrite On
				ColorMask 0
				Cull[_Cull]

				HLSLPROGRAM
				// Required to compile gles 2.0 with standard srp library
				#pragma prefer_hlslcc gles
				#pragma exclude_renderers d3d11_9x
				#pragma target 2.0

				#pragma vertex DepthOnlyVertex
				#pragma fragment DepthOnlyFragment

				// -------------------------------------
				// Material Keywords
				#pragma shader_feature _ALPHATEST_ON
				#pragma shader_feature _GLOSSINESS_FROM_BASE_ALPHA

				//--------------------------------------
				// GPU Instancing
				#pragma multi_compile_instancing

				#include "Packages/com.unity.render-pipelines.universal/Shaders/SimpleLitInput.hlsl"
				#include "Packages/com.unity.render-pipelines.universal/Shaders/DepthOnlyPass.hlsl"
				ENDHLSL
			}
			}
}
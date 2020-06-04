Shader "my_shader/skin_specular"
{
	Properties
	{
		_albedo_tex ("albedo texture", 2D) = "white" {}
		_normal_tex ("normal texture", 2D) = "bump"{}
		_mix_tex ("mix texture (R metallic, G roughness)", 2D) = "black" {}
		[HDR]_emissive("Emissive", Color) = (0.0, 0.0, 0.0, 0.0)
		[KeywordEnum(DEFAULT, SUBSURFACE, SKIN, HAIR, HAIR_UE)] _LIGHTING_TYPE("shading model", Float) = 0

			//在开启 SUBSURFACE的情况下,_sss_color作为次表面散射的色彩
			_sss_color("SSS color", Color) = (0.0, 0.0, 0.0, 1.0)
			_sss_strength("sss strength", Range(0,100)) = 0.0
			_sss_power("sss power", Range(0, 100)) = 5.0

			//skin相关
			_preinteger_tex("preinteger tex", 2D) = "white" {}
			_sss_tex("sss tex", 2D) = "white" {}

			//hair相关
			_anisotropy("anisortopy", Range(-1.0, 1.0)) = 0.0
			_anisotropy_intensity("_anisotropy_intensity", Range(0.1, 10.0)) = 1.0
			_hair_jitter("hair jitter", 2D) = "black" {}
			_jitter_scale("jitter scale", Range(0, 10)) = 0.0
			_hair_tangent("hair tangent", 2D) = "white" {}

			//ue_hair相关
			_ue_hair_tex("UE hair tex, R-Depth G-ID B-Root A-Alpha", 2D) = "white" {}
			_root_color("root color", Color) = (0,0,0,1)
			_tip_color("tip color", Color) = (0,0,0,1)
			_roughness_range("roughness range", Vector) = (0.3, 0.5,0,0)
			_metallic_range("metallic range", Vector) = (0.1, 0.2, 0, 0)
			_hair_clip_alpha("hair clip alpha", Range(0, 1)) = 0.5
			_hair_specular_color("hair specular color", Color) = (1.0, 1.0, 1.0, 1.0)
			_hair_depth_unit("hair depth unit", Float) = 1.0


			//color_tint
			/*
			_id_tex("tint mask texture", 2D) = "black" {}
			_color_tint1("color tint1", Color) = (0.0, 0.0, 0.0, 0.0)
			_color_tint2("color tint2", Color) = (0.0, 0.0, 0.0, 0.0)
			_color_tint3("color tint3", Color) = (0.0, 0.0, 0.0, 0.0)
			*/

			//emissive
			//_emissive_mask_map("Emissive mask map", 2D) = "black" {}

			//fresnel
			/*
			_fresnel_color("fresnel color", Color) = (0.0, 0.0, 0.0, 0.0)
			_fresnel_scale("fresnel scale", Range(0, 1)) = 0.0
			_fresnel_bias("fresnel bias", Range(-1, 1)) = 0.0
			_fresnel_power("fresnel power", Range(0, 10)) = 5.0
			*/

			//energy
			/*
			_energy_tex("energy tex", 2D) = "black" {}
			_mask_tex("mask tex", 2D) = "white" {}
			_speed_x("speed X", Float) = 0.0
			_speed_y("speed Y", Float) = 0.0
			_energy_strength("energy strength", Range(0.1, 10) ) = 1.0
			_pos_scale("pos scale value", Vector) = (1.0, 1.0, 1.0, 0.0)
			*/

			//bump
			/*
			_bump_tex("bump tex", 2D) = "black" {}
			_bump_scale("bump scale", Range(-3, 3)) = 1.0
			_bump_tex_size("bump tex size", Vector) = (512, 512, 0, 0)
			*/

			// distortion
			/*
			_distortion_tex("distortion tex", 2D) = "black" {}
			_speeds("speeds", Vector) = (0.0, 0.0, 0.0, 0.0)
			_distortion_strength("distortion strength", Range(0.0, 10.0)) = 0.0
			*/

			// dissolved
			/*
			_dissolved_tex("dissolved tex", 2D) = "white" {}
			_alpha_ref("alpha ref", Range(0,1)) = 0.0
			_alpha_width("alpha width", Range(0,1)) = 0.0
			[HDR]_highlight_color("highlight color", Color) = (0.0, 0.0, 0.0, 0.0)
			*/
	}
		SubShader
		{
			// 这里的tags要这么写，不然阴影会有问题
			Tags { "RenderType" = "Opaque" "Queue" = "Geometry"}
			LOD 100
			Pass
			{
			// 这个ForwardBase非常重要，不加这个， 光照取的结果都会跳变……
			Tags {"LightMode" = "ForwardBase"}
			CGPROGRAM
			#pragma target 5.0
			#pragma vertex vert
			#pragma fragment frag
			#pragma multi_compile LIGHTMAP_OFF LIGHTMAP_ON
			#pragma multi_compile DYNAMICLIGHTMAP_OFF DYNAMICLIGHTMAP_ON
			#pragma multi_compile __ DIRLIGHTMAP_COMBINED
			//#pragma multi_compile __ UNITY_SPECCUBE_BOX_PROJECTION   //奇怪了，不开启这个也能生效，环境球反射……
			#pragma multi_compile __ LIGHTPROBE_SH

			#pragma multi_compile_fwdbase
			#pragma enable_d3d11_debug_symbols
			#pragma shader_feature _LIGHTING_TYPE_DEFAULT _LIGHTING_TYPE_SUBSURFACE _LIGHTING_TYPE_SKIN _LIGHTING_TYPE_HAIR _LIGHTING_TYPE_HAIR_UE

			float _sss_strength;
			float _anisotropy;
			float _anisotropy_intensity;
			sampler _hair_jitter;
			float _jitter_scale;
			sampler _hair_tangent;

			// ue hair
			sampler _ue_hair_tex;
			float4 _root_color;
			float4 _tip_color;
			float4 _roughness_range;
			float4 _metallic_range;
			float _hair_clip_alpha;
			float4 _hair_specular_color;
			float _hair_depth_unit;
			
			#include "UnityCG.cginc"
			#include "Lighting.cginc"
			#include "AutoLight.cginc"

			#include "common.cginc"
			#include "brdf.cginc"
			#include "gi_lighting.cginc"

			#include "effects.cginc"

			v2f vert (appdata v)
			{
				v2f o;
				o.pos = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;

				o.world_pos = mul(unity_ObjectToWorld, v.vertex).xyz;
				o.world_normal = mul(v.normal.xyz, (float3x3)unity_WorldToObject);
				o.world_tangent = normalize(mul((float3x3)unity_ObjectToWorld, v.tangent.xyz));
				o.world_binnormal = cross(o.world_normal, o.world_tangent)*v.tangent.w;

   				#ifdef LIGHTMAP_ON
					o.lightmap_uv.xy = v.uv1.xy * unity_LightmapST.xy + unity_LightmapST.zw;
					o.lightmap_uv.zw = 0;
				#endif

				#ifdef DYNAMICLIGHTMAP_ON
					o.lightmap_uv.zw = v.uv2.xy * unity_DynamicLightmapST.xy + unity_DynamicLightmapST.zw;
				#endif

				o.screen_pos = ComputeScreenPos(o.pos);
				TRANSFER_SHADOW(o);

				return o;
			}

			MaterialVars gen_material_vars(v2f i)
			{
				MaterialVars mtl;
				float4 albedo_color =  tex2D(_albedo_tex, i.uv);
				mtl.albedo = albedo_color.rgb;

				float3 normal_color = tex2D(_normal_tex, i.uv).rgb;
				mtl.normal = normal_color*2.0 - 1.0;
				mtl.roughness = tex2D(_mix_tex, i.uv).g; //_roughness;
				mtl.metallic = tex2D(_mix_tex, i.uv).r; //_metallic;
				mtl.emissive = _emissive;
				mtl.opacity = albedo_color.a;
				mtl.occlusion = 1.0;

				mtl.sss_color = _sss_color.rgb;

				float4 sss_tex_data = tex2D(_sss_tex, i.uv);
				mtl.thickness = sss_tex_data.r;
				mtl.curvature = sss_tex_data.g;
				return mtl;
			}

			LightingVars gen_lighting_vars(v2f i, MaterialVars mtl)
			{
				LightingVars data;
				data.T = normalize(i.world_tangent);
				data.B = normalize(i.world_binnormal);
				data.N = normalize(normalize(i.world_tangent) * mtl.normal.x + normalize(i.world_binnormal) * mtl.normal.y + normalize(i.world_normal) * mtl.normal.z);

				data.V = normalize(_WorldSpaceCameraPos.xyz - i.world_pos.xyz);
				data.L = normalize(_WorldSpaceLightPos0.xyz);
				data.H = normalize(data.V + data.L);
				data.diffuse_color = mtl.albedo*(1.0 - mtl.metallic);
				data.f0 = lerp(float3(0.04, 0.04, 0.04), mtl.albedo, mtl.metallic);
				data.roughness = mtl.roughness;
				data.metallic = mtl.metallic;
				data.sss_color = mtl.sss_color;
				data.thickness = mtl.thickness;
				data.curvature = mtl.curvature;
				data.opacity = mtl.opacity;

				data.light_color = _LightColor0.rgb;

				#if defined(LIGHTMAP_ON) || defined(DYNAMICLIGHTMAP_ON)
					data.lightmap_uv = i.lightmap_uv;
				#endif

				data.world_pos = i.world_pos;

				data.occlusion = mtl.occlusion;
				data.shadow = UNITY_SHADOW_ATTENUATION(i, data.world_pos);

				data.base_vars.pos = i.pos;
				data.base_vars.uv0 = i.uv;
				data.pos = i.pos;
				return data;
			}

			void Unity_Dither(float alpha, float2 ScreenPosition)
			{
				float2 uv = ScreenPosition * _ScreenParams.xy + _Time.yz * 100;
				float DITHER_THRESHOLDS[16] =
				{
					1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
					13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
					4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
					16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
				};
				uint index = (uint(uv.x) % 4) * 4 + uint(uv.y) % 4;
				clip(alpha - DITHER_THRESHOLDS[index]);
			}

			fixed4 frag (v2f i, out float depth:SV_Depth) : SV_Target
			//fixed4 frag(v2f i) : SV_Target
			{
				MaterialVars mtl = gen_material_vars(i);
				LightingVars data = gen_lighting_vars(i, mtl);
				//effect_color_tint(i, mtl, data);
				//effect_energy(i, mtl, data);
				//effect_energy_model_space(i, mtl, data);
				//effect_bump(i, mtl, data);
				//effect_distortion(i, mtl, data);
				//effect_dissovle(i, mtl, data);

				data = gen_lighting_vars(i, mtl);

				// lighting part
				//LightingResult dir_result = direct_blinnphone_lighting(data);
				LightingResult dir_result = direct_lighting(data);
				
				fixed3 final_color = dir_result.lighting_specular*data.shadow; // (dir_result.lighting_diffuse + dir_result.lighting_specular)*data.shadow + dir_result.lighting_scatter;
				//GI的处理
				/*
				LightingResult gi_result = gi_lighting(data);

				#ifndef _LIGHTING_TYPE_HAIR_UE
					final_color = final_color + (gi_result.lighting_diffuse + gi_result.lighting_specular)*data.occlusion + mtl.emissive;
					depth = data.pos.z/data.pos.w;
				#else
					float4 ue_hair_data = tex2D(_ue_hair_tex, i.uv);
					float2 screen_pos = i.screen_pos.xy / i.screen_pos.w;
					Unity_Dither(ue_hair_data.a - _hair_clip_alpha, screen_pos);					
					depth = saturate( data.pos.z/data.pos.w + ue_hair_data.r*_hair_depth_unit);
				#endif
				*/
				// sample the texture
				return fixed4(final_color, mtl.opacity);
			}
			ENDCG
		}


		// Pass to render object as a shadow caster
		
		Pass {
			Tags { "LightMode" = "ShadowCaster" }
			
			CGPROGRAM
			
			#pragma vertex vert
			#pragma fragment frag
			
			#pragma multi_compile_shadowcaster
			
			#include "UnityCG.cginc"
			
			struct v2f {
				V2F_SHADOW_CASTER;
				float2 uv:TEXCOORD1;
			};
			
			v2f vert(appdata_base v) {
				v2f o;
				
				TRANSFER_SHADOW_CASTER_NORMALOFFSET(o)
				
				o.uv = v.texcoord;
				
				return o;
			}
			
			fixed4 frag(v2f i) : SV_Target {				
				SHADOW_CASTER_FRAGMENT(i)
			}
			ENDCG
		}
		
	}

}

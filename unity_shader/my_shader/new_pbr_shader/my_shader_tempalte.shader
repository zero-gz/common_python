﻿Shader "my_shader/my_shader_tempalte"
{
	Properties
	{
		_albedo_tex ("albedo texture", 2D) = "white" {}
		_normal_tex ("normal texture", 2D) = "bump"{}
		_mix_tex ("mix texture (R metallic, G roughness)", 2D) = "black" {}
		[HDR]_emissive ("Emissive", Color) = (0.0, 0.0, 0.0, 0.0)

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
		_energy_tex("energy tex", 2D) = "black" {}
		_mask_tex("mask tex", 2D) = "white" {}
		_speed_x("speed X", Float) = 0.0
		_speed_y("speed Y", Float) = 0.0
		_energy_strength("energy strength", Range(0.1, 10) ) = 1.0
		_pos_scale("pos scale value", Vector) = (1.0, 1.0, 1.0, 0.0)
	}
	SubShader
	{
		// 这里的tags要这么写，不然阴影会有问题
		Tags { "RenderType"="Opaque" "Queue"="Geometry"}
		LOD 100

		Pass
		{
			// 这个ForwardBase非常重要，不加这个， 光照取的结果都会跳变……
			Tags {"LightMode"="ForwardBase"}
			CGPROGRAM
			#pragma target 3.0
			#pragma vertex vert
			#pragma fragment frag
			#pragma multi_compile LIGHTMAP_OFF LIGHTMAP_ON
			#pragma multi_compile DYNAMICLIGHTMAP_OFF DYNAMICLIGHTMAP_ON
			#pragma multi_compile __ DIRLIGHTMAP_COMBINED
			//#pragma multi_compile __ UNITY_SPECCUBE_BOX_PROJECTION   //奇怪了，不开启这个也能生效，环境球反射……
			#pragma multi_compile __ LIGHTPROBE_SH

			#pragma multi_compile_fwdbase
			#pragma enable_d3d11_debug_symbols
			
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

				TRANSFER_SHADOW(o);

				return o;
			}

			MaterialVars gen_material_vars(v2f i)
			{
				MaterialVars mtl;
				mtl.albedo =  tex2D(_albedo_tex, i.uv).rgb;

				float3 normal_color = tex2D(_normal_tex, i.uv).rgb;
				mtl.normal = normal_color*2.0 - 1.0;
				mtl.roughness = tex2D(_mix_tex, i.uv).g; //_roughness;
				mtl.metallic = tex2D(_mix_tex, i.uv).r; //_metallic;
				mtl.emissive = _emissive;
				mtl.opacity = 1.0;
				mtl.occlusion = 1.0;

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
				data.light_color = _LightColor0.rgb;

				#if defined(LIGHTMAP_ON) || defined(DYNAMICLIGHTMAP_ON)
					data.lightmap_uv = i.lightmap_uv;
				#endif

				data.world_pos = i.world_pos;

				data.occlusion = mtl.occlusion;
				data.shadow = UNITY_SHADOW_ATTENUATION(i, data.world_pos);

				data.base_vars.pos = i.pos;
				data.base_vars.uv0 = i.uv;
				return data;
			}

			void effect_modify_vars(inout MaterialVars mtl, inout LightingVars data)
			{
				//data.diffuse_color = float3(1.0,0.0,0.0);
			}

			fixed4 frag (v2f i) : SV_Target
			{
				MaterialVars mtl = gen_material_vars(i);
				LightingVars data = gen_lighting_vars(i, mtl);
				//effect_color_tint(i, mtl, data);
				//effect_energy(i, mtl, data);
				effect_energy_model_space(i, mtl, data);
				data = gen_lighting_vars(i, mtl);

				// lighting part
				//LightingResult dir_result = direct_blinnphone_lighting(data);
				LightingResult dir_result = direct_isotropy_lighting(data);
				
				fixed3 final_color = dir_result.lighting_diffuse + dir_result.lighting_specular;
				final_color *= data.shadow;
				
				// unity的问题，乘以了pi
				final_color = final_color * PI;

				//GI的处理
				LightingResult gi_result = gi_lighting(data);

				final_color = final_color + (gi_result.lighting_diffuse + gi_result.lighting_specular)*data.occlusion;

				//final_color = abs(i.pos)/3.0f;
				// sample the texture
				return fixed4(final_color, 1.0);
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

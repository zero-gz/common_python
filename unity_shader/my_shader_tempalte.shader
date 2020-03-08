Shader "my_shader/my_shader_tempalte"
{
	Properties
	{
		_albedo_tex ("albedo texture", 2D) = "white" {}
		_normal_tex ("normal texture", 2D) = "bump"{}
		_mix_tex ("mix texture (R metallic, G roughness)", 2D) = "black" {}
		//_roughness ("Roughness", Range(0.0, 1.0)) = 0.0
		//_metallic ("Metallic", Range(0.0, 1.0)) = 0.0
		_emissive ("Emissive", Color) = (0.0, 0.0, 0.0, 0.0)
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

			sampler2D _albedo_tex;
			sampler2D _normal_tex;
			sampler2D _mix_tex;

			float _roughness;
			float _metallic;
			float3 _emissive;

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

			struct LightingResult{
				float3 lighting_diffuse;
				float3 lighting_specular;
			};

			float Pow2(float c)
			{
				return c*c;
			}

			#define PI 3.1415926
			#define BLACK_COLOR float3(0.0, 0.0, 0.0)
			#define WHITE_COLOR float3(1.0, 1.0, 1.0)
			#define CHAOS 0.000001	

			float3 gamma_correct_began(float3 input_color)
			{
				return input_color*input_color;
			}		

			float3 gamma_correct_end(float3 input_color)
			{
				return sqrt(input_color);
			}

			MaterialVars gen_material_vars(v2f i)
			{
				MaterialVars mtl;
				mtl.albedo =  gamma_correct_began(tex2D(_albedo_tex, i.uv).rgb);

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
				return data;
			}

			LightingResult direct_blinnphone_lighting(LightingVars data)
			{
				LightingResult result;
				result.lighting_diffuse = data.light_color*data.diffuse_color*max(dot(data.N, data.L), 0.0);
				result.lighting_specular = data.light_color*data.f0*pow(max(dot(data.H, data.N), 0.0), 32) ;
				return result;
			}

//   ##################################################################################################

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

//  ####################################################################################################
			void init_result(inout LightingResult result)
			{
				result.lighting_diffuse = float3(0.0, 0.0, 0.0);
				result.lighting_specular = float3(0.0, 0.0, 0.0);
			}

			float3 ibl_lighting_diffuse(LightingVars data)
			{
				return data.diffuse_color * ShadeSH9(float4(data.N, 1.0));
			}

			// Env BRDF Approx
			float3 env_approx(LightingVars data)
			{
				float NoV = max(saturate(dot(data.N, data.V)), CHAOS);

				float4 C0 = float4(-1.000f, -0.0275f, -0.572f,  0.022f);
				float4 C1 = float4(1.000f,  0.0425f,  1.040f, -0.040f);
				float2 C2 = float2(-1.040f,  1.040f);
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
				half4 rgbm = UNITY_SAMPLE_TEXCUBE_LOD(unity_SpecCube0, reflectVec, mip);

				float3 iblSpecular = DecodeHDR(rgbm, unity_SpecCube0_HDR);

				// ibl specular part2
				float3 brdf_factor = env_approx(data);

				return iblSpecular*brdf_factor;
			}

			float3 lightmap_lighting_diffuse(LightingVars data)
			{
				float3 lightmap_baked = float3(0.0, 0.0, 0.0);
				float3 lightmap_realtime = float3(0.0, 0.0, 0.0);
				#if defined(LIGHTMAP_ON)
					// Baked lightmaps
					half4 bakedColorTex = UNITY_SAMPLE_TEX2D(unity_Lightmap, data.lightmap_uv.xy);
					half3 bakedColor = DecodeLightmap(bakedColorTex);

					#ifdef DIRLIGHTMAP_COMBINED
						fixed4 bakedDirTex = UNITY_SAMPLE_TEX2D_SAMPLER (unity_LightmapInd, unity_Lightmap, data.lightmap_uv.xy);
						lightmap_baked += DecodeDirectionalLightmap (bakedColor, bakedDirTex, data.N);
					#else
						lightmap_baked += bakedColor;	
					#endif
				#endif

				#ifdef DYNAMICLIGHTMAP_ON
					// Dynamic lightmaps
					fixed4 realtimeColorTex = UNITY_SAMPLE_TEX2D(unity_DynamicLightmap, data.lightmap_uv.zw);
					half3 realtimeColor = DecodeRealtimeLightmap (realtimeColorTex);

					#ifdef DIRLIGHTMAP_COMBINED
						half4 realtimeDirTex = UNITY_SAMPLE_TEX2D_SAMPLER(unity_DynamicDirectionality, unity_DynamicLightmap, data.lightmap_uv.zw);
						lightmap_realtime += DecodeDirectionalLightmap (realtimeColor, realtimeDirTex, data.N);
					#else
						lightmap_realtime += realtimeColor;
					#endif
				#endif

				return (lightmap_baked + lightmap_realtime)*data.diffuse_color;
			}


			LightingResult gi_lighting(LightingVars data)
			{
				LightingResult result;
				init_result(result);
				
				#ifdef LIGHTPROBE_SH
					result.lighting_diffuse += ibl_lighting_diffuse(data);
				#endif

				#ifdef UNITY_SPECCUBE_BOX_PROJECTION
					result.lighting_specular += ibl_lighting_specular(data);
				#endif	

				#if defined(LIGHTMAP_ON) || defined(DYNAMICLIGHTMAP_ON)
					result.lighting_diffuse += lightmap_lighting_diffuse(data);
				#endif

				return result;
			}


//  #####################################################################################################
			fixed4 frag (v2f i) : SV_Target
			{
				MaterialVars mtl = gen_material_vars(i);
				LightingVars data = gen_lighting_vars(i, mtl);

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

				final_color = gamma_correct_end(final_color);

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

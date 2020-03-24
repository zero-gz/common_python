Shader "my_shader/image_simple"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_Brightness ("Brightness", Range(0, 10) ) = 1.0
		_Saturation ("Saturation", Range(-10, 10) ) = 1.0
		_Constrast("Constrast", Range(0, 10) ) = 1.0

		_move_x ("Move X speed", Float) = 0.0
		_move_y ("Move Y speed", Float) = 0.0	

		_blur_size("Blur Size", Float) = 0.1
		_fuzzy_dir("Fuzzy Dir", Vector) = (0, 0, 0, 0)
		_sample_count("Sample Count", Range(0, 10) ) = 5
	}
	SubShader
	{
		Tags { "RenderType"="Opaque" }
		LOD 100

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"


			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
			};

			sampler2D _MainTex;
			float4 _MainTex_ST;

			float _Brightness;
			float _Saturation;
			float _Constrast;

			float _move_x;
			float _move_y;

			float _blur_size;
			float4 _fuzzy_dir;
			float _sample_count;

#define MID_FUZZY_AVG 1.0/9.0
			// do some fuzzy
			float3 apply_mid_fuzzy(float2 uv, sampler2D tex)
			{
				float3 color_out = float3(0.0, 0.0, 0.0);
				for(int i=-1;i<=1;i++)
					for (int j = -1; j <= 1; j++)
					{
						float2 aim_uv = uv + float2(i, j)*_blur_size;
						color_out += tex2D(tex, aim_uv).rgb;
					}

				color_out = color_out / 9.0;
				return color_out;
			}

			float3 apply_dir_fuzzy(float2 uv, float2 dir, int sample_count, sampler2D tex)
			{
				float adjust_size[10] = {1,1,2,2,2,4,4,4,4,4};
				dir = normalize(dir);
				float3 color_out = tex2D(tex, uv).rgb;
				for (int i = 1; i < sample_count; i++)
				{
					float2 aim_uv = uv + dir*i*_blur_size*adjust_size[i];
					color_out += tex2D(tex, aim_uv).rgb;
				}

				color_out = color_out / sample_count;
				return color_out;
			}

			// can do color grading process
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				return o;
			}

			float3 apply_image_bsc(float3 color)
			{
				// Apply brightness
				float3 final_color = color.rgb * _Brightness;

				// Apply saturation
				float luminance = 0.2125 * color.r + 0.7154 * color.g + 0.0721 * color.b;
				float3 luminanceColor = float3(luminance, luminance, luminance);
				final_color = lerp(luminanceColor, final_color, _Saturation);

				// Apply contrast
				float3 avgColor = float3(0.5, 0.5, 0.5);
				final_color = lerp(avgColor, final_color, _Constrast);
				return final_color;
			}

			float2 apply_uv_trans(float2 uv)
			{
				float2 new_uv;
				new_uv = uv + float2(_move_x, _move_y)*_Time.y;
				return new_uv;
			}
			
			float4 frag (v2f i) : SV_Target
			{
				// sample the texture
				/*
				float2 uv = apply_uv_trans(i.uv);

				float4 color = tex2D(_MainTex, uv);
				float3 out_col = apply_image_bsc(color);

				return float4(out_col.rgb, color.a);
				*/

				
				//float3 out_col = apply_mid_fuzzy(i.uv, _MainTex);

				float2 uv = apply_uv_trans(i.uv);
				float3 out_col = apply_dir_fuzzy(uv, _fuzzy_dir.xy, _sample_count, _MainTex);
				return float4(out_col.rgb, 1.0f);
			}
			ENDCG
		}
	}
}

Shader "my_shader/image_bsc"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_Brightness ("Brightness", Range(0, 10) ) = 1.0
		_Saturation ("Saturation", Range(-10, 10) ) = 1.0
		_Constrast("Constrast", Range(0, 10) ) = 1.0

		_move_x ("Move X speed", Float) = 0.0
		_move_y ("Move Y speed", Float) = 0.0	
	}
	SubShader
	{
		Tags{ "Queue" = "Transparent" "IgnoreProjector" = "True" "RenderType" = "Transparent" "DisableBatching" = "True" }

		Pass
		{
			Tags{ "LightMode" = "ForwardBase" }

			ZWrite Off
			Blend SrcAlpha OneMinusSrcAlpha
			Cull back

			LOD 100

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

			// can do color grading process
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);

				//o.uv = o.uv * float2(-1, -1);
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
				
				float2 uv = apply_uv_trans(i.uv);

				float4 color = tex2D(_MainTex, uv);
				float3 out_col = apply_image_bsc(color);

				return float4(out_col.rgb, color.a);
			}
			ENDCG
		}
	}
}

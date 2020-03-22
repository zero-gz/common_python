Shader "my_shader/test_paint"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_paint ("Paint Data", Vector) = (100, 100, 0.5, 1.0)
		_color ("Paint Color", Color) = (1.0, 0.0, 0.0, 1.0)
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
			#pragma enable_d3d11_debug_symbols
			
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
			float4 _paint;
			float4 _color;
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				return o;
			}

			fixed4 draw_point(fixed4 col, float2 now_screen_pos, float2 aim_screen_pos, float blur)
			{
				float dist = length(aim_screen_pos - now_screen_pos);
				fixed4 out_col = lerp(_color, col, abs(dist-blur)/blur );
				return out_col;
			}
			
			fixed4 frag (v2f i) : SV_Target
			{
				// sample the texture
				fixed4 col = tex2D(_MainTex, i.uv);

				col = draw_point(col, i.uv*_ScreenParams.xy, _paint.xy, _paint.z);
				return col;
			}
			ENDCG
		}
	}
}

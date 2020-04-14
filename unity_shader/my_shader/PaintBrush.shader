// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "my_shader/PaintBrush"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}
		_color("Color",Color)=(1,1,1,1)
		_UV("UV",Vector)=(0,0,0,0)
		_paint_size("Paint Size(uv coord)", Float) = 0.02
		_paint_stength("Paint Stength", Range(0.1, 1.0)) = 1.0
	}
	SubShader
	{
		Tags { "RenderType"="Transparent" }
		LOD 100
		ZTest Always Cull Off ZWrite Off Fog{ Mode Off }
		Blend SrcAlpha OneMinusSrcAlpha
		//Blend One DstColor
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
			fixed4 _UV;
			fixed4 _color;
			float _paint_size;
			float _paint_stength;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				return o;
			}

			float4 draw_cicle(float2 uv, float2 aim_uv)
			{
				float2 diff = uv - aim_uv;
				float dist = length(diff);				

				if (dist < _paint_size)
				{
					//float alpha = 1.0f - smoothstep(_paint_stength, 1.0, dist / _paint_size);
					float alpha = 1.0f;
					return float4(_color.rgb*alpha, 1.0f);
				}
				return float4(0.0, 0.0, 0.0, 0.0);
			}
			
			fixed4 frag (v2f i) : SV_Target
			{
				fixed4 col = tex2D(_MainTex, i.uv);
				float4 out_color = draw_cicle(i.uv, _UV.xy);
				float4 final_color = col + out_color;
				return final_color;
			}
			ENDCG
		}
	}
}

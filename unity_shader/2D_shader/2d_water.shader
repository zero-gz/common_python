Shader "my_shader/2d_water"
{
	Properties
	{
		_MainTex ("Main Texture", 2D) = "white" {}
		_distortion_tex ("Distortion Texture", 2D) = "white" {}
		_speed_x ("Speed X", Float) = 0.0
		_speed_y ("Speed Y", Float) = 0.0
		_strength("Distortion Strength", Range(0.0, 2.0) ) = 0.0
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
				float2 distortion_uv: TEXCOORD1;
			};

			sampler2D _MainTex;
			float4 _MainTex_ST;

			sampler2D _distortion_tex;
			float4 _distortion_tex_ST;

			float _speed_x;
			float _speed_y;
			float _strength;
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				o.distortion_uv = TRANSFORM_TEX(v.uv, _distortion_tex) + _Time.x*float2(_speed_x, _speed_y);

				//o.uv = o.uv*float2(-1, -1);
				//o.distortion_uv = o.distortion_uv*float2(-1, -1);
				return o;
			}

			float2 apply_distortion_uv(sampler2D dis_tex, float2 uv)
			{
				float2 dis_uv = tex2D(dis_tex, uv).xy;
				return dis_uv*2.0f - 1.0f;
			}
			
			fixed4 frag (v2f i) : SV_Target
			{
				// sample the texture
				float2 new_uv = i.uv + apply_distortion_uv(_distortion_tex, i.distortion_uv)*_strength;
				new_uv.y = -new_uv.y;
				fixed4 col = tex2D(_MainTex, new_uv);
				return col;
			}
			ENDCG
		}
	}
}

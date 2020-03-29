// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "my_shader/2d_cloud"
{
	Properties
	{
		_Octave0("Octave0", 2D) = "white" {}
	_Octave1("Octave1", 2D) = "white" {}
	_Octave2("Octave2", 2D) = "white" {}
	_Octave3("Octave3", 2D) = "white" {}

	_Speed("_Speed", Float) = 1.0
		_Emptiness("_Emptiness", Float) = 0.0
		_Sharpness("_Sharpness", Float) = 1.0
		_CloudColor("_CloudColor", Color) = (1.0, 1.0, 1.0, 1.0)
	}
		SubShader
	{
		Tags{ "Queue" = "Transparent" "IgnoreProjector" = "True" "RenderType" = "Transparent" "DisableBatching" = "True" }
		LOD 100

		Pass
		{
		ZWrite Off
		Blend SrcAlpha OneMinusSrcAlpha
		Cull back

			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag

			#include "UnityCG.cginc"
			sampler2D _Octave0;
			sampler2D _Octave1;
			sampler2D _Octave2;
			sampler2D _Octave3;
			float4 _Octave0_ST;
			float4 _Octave1_ST;
			float4 _Octave2_ST;
			float4 _Octave3_ST;
			float _Speed;

			float _Emptiness;
			float _Sharpness;
			float4 _CloudColor;

			struct appdata
			{
				float4 vertex : POSITION;
				float2 texcoord : TEXCOORD0;
			};

			struct v2f
			{
				float4 uv0 : TEXCOORD0;
				float4 uv1 : TEXCOORD1;
				float4 pos : SV_POSITION;
			};


			v2f vert(appdata v) {
				v2f o;
				o.pos = UnityObjectToClipPos(v.vertex);
				o.uv0.xy = TRANSFORM_TEX(v.texcoord, _Octave0) + _Time.x * 1.0 * _Speed * half2(1.0, 0.0);
				o.uv0.zw = TRANSFORM_TEX(v.texcoord, _Octave1) + _Time.x * 1.5 * _Speed * half2(0.0, 1.0);
				o.uv1.xy = TRANSFORM_TEX(v.texcoord, _Octave2) + _Time.x * 2.0 * _Speed * half2(0.0, -1.0);
				o.uv1.zw = TRANSFORM_TEX(v.texcoord, _Octave3) + _Time.x * 2.5 * _Speed * half2(-1.0, 0.0);
				return o;
			}

			fixed4 frag(v2f i) : SV_Target
			{
				float4 n0 = tex2D(_Octave0, i.uv0.xy);
				float4 n1 = tex2D(_Octave1, i.uv0.zw);
				float4 n2 = tex2D(_Octave2, i.uv1.xy);
				float4 n3 = tex2D(_Octave3, i.uv1.zw);

				float4 fbm = 0.5 * n0 + 0.25 * n1 + 0.125 * n2 +0.0625 * n3;
				fbm = (clamp(fbm, _Emptiness, _Sharpness) - _Emptiness) / (_Sharpness - _Emptiness);

				/*
				fixed4 ray = fixed4(0.0, 0.2, 0.4, 0.6);
				fixed amount = dot(max(fbm - ray, 0), fixed4(0.25, 0.25, 0.25, 0.25));
				fixed4 col;
				col.rgb = amount * _CloudColor.rgb + 2.0 * (1.0 - amount) * 0.4;
				col.a = amount * 1.5;
				*/


				fixed4 col;
				col.rgb = _CloudColor.rgb;
				col.a = fbm;
				
				//col = float4(fbm.r, fbm.r, fbm.r, 1.0f);
				return col;
			}
			ENDCG
		}
	}
}

// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "my_shader/draw_diffuse_scatter" {
	Properties{
		factor1("factor1", Vector) = (0.064, 0.233, 0.455, 0.649)
		factor2("factor2", Vector) = (0.0484, 0.100, 0.336, 0.344)
		factor3("factor3", Vector) = (0.1870, 0.118, 0.198, 0.000)
		factor4("factor4", Vector) = (0.5670, 0.113, 0.007, 0.007)
		factor5("factor5", Vector) = (1.9900, 0.358, 0.004, 0.000)
		factor6("factor6", Vector) = (7.4100, 0.078, 0.000, 0.000)
	}
	SubShader{
		Tags { "RenderType" = "Opaque" }
		LOD 200

		Pass {

		Tags { "LightMode" = "Always"}
		ZTest Always
		Cull Off
		ZWrite Off
		Blend Off

		CGPROGRAM
		#pragma vertex vert
		#pragma fragment frag
		#pragma target 3.0
		#pragma enable_d3d11_debug_symbols
		#include "UnityCG.cginc"
		#define PI 3.1415926

		float4 factor1;
		float4 factor2;
		float4 factor3;
		float4 factor4;
		float4 factor5;
		float4 factor6;

		struct v2f {
			float4 pos : SV_POSITION;
			float4 uv : TEXCOORD0;
		};

		v2f vert(appdata_base v) {
			v2f o;
			o.pos = UnityObjectToClipPos(v.vertex);
			o.uv = v.texcoord;
			return o;
		}

		float Gaussian(float v, float r)
		{
			return 1.0f / sqrt(2.0f * PI * v) * exp(-(r * r) / (2.0f * v));
			//return 1.0f / (2.0f * PI * v) * exp(-(r * r) / (2.0f * v));
		}

		float3 Scatter(float r)
		{
			float sq = 1.414f;

			return Gaussian(factor1.x*sq, r) * factor1.yzw +
				Gaussian(factor2.x*sq, r) * factor2.yzw +
				Gaussian(factor3.x*sq, r) * factor3.yzw +
				Gaussian(factor4.x*sq, r) * factor4.yzw +
				Gaussian(factor5.x*sq, r) * factor5.yzw +
				Gaussian(factor6.x*sq, r) * factor6.yzw;
		}

		float4 frag(v2f i) : COLOR {
			float x = (i.uv.x - 0.5) + 0.5;
			float y = i.uv.y;

			float r = distance(float2(x,y), float2(0.5, 0.5));
			r *= 10.0;
			r = max(0.0, r - 1.0);
			float3 profile = Scatter(r);
			profile = LinearToGammaSpace(profile);

			return float4(profile, 1.0);
		}
	ENDCG
}
}
FallBack Off
}
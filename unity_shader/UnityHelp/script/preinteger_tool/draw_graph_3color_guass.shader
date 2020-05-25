Shader "my_shader/draw_graph_3color_guass"
{
	Properties{
		_tex_size("Tex Size", Float) = 256.0
		_x_size("X size", Range(1, 20)) = 1.0
		_y_size("Y size", Range(0.1, 20)) = 1.0
		_back_color("back color", Color) = (0.0, 0.0, 0.0, 1.0)
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

			float _tex_size;
			float _x_size;
			float _y_size;
			float4 _back_color;

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

			float3 plot(float tmp_x) {
				float x = tmp_x * _x_size;
				// 这里写计算公式
				/*
				float y0 = sin(x);
				float y1 = cos(x);
				float y2 = x;
				*/

				float3 y_result = Scatter(x);
				y_result = (y_result / _y_size + float3(1.0, 1.0, 1.0)) / 2.0;
				return y_result;
			}

			float4 frag(v2f i) : COLOR{
				float x = i.uv.x;
				float y = i.uv.y;

				float pixelWidth = 1.0 / _tex_size;
				float pixelHeight = 1.0 / _tex_size;
				float3 color = _back_color;
				for (int rgb = 0; rgb < 3; rgb++) {
					int samples = 4;
					float2 step = float2(pixelWidth / samples * 2.0, pixelHeight / samples * 2.0);
					float sum = 0.0;
					for (int i = 0.0; i < samples; i++) {
						for (int j = 0.0; j < samples; j++) {
							//注意这里的括号,是取数组的值
							float f = plot(x + i * step.x)[rgb] - (y + j * step.y);
							sum += (f > 0.) ? 1 : -1;
						}
					}

					float alpha = 1.0 - abs(sum) / (samples*samples);
					color = lerp(color, float3(rgb == 0, rgb == 1, rgb == 2), saturate(alpha));
				}
				return float4(color, 1.0);
			}
			ENDCG
			}
	}
		FallBack Off
}

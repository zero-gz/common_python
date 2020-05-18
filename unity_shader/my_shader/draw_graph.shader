Shader "my_shader/draw_graph"
{
	Properties{
		_tex_size("Tex Size", Float) = 512.0
		_x_size("X size", Range(1, 20)) = 1.0
		_y_size("Y size", Range(1, 20)) = 1.0
		_line_color("line color", Color) = (1.0, 0.0, 0.0, 1.0)
		_back_color("back color", Color) = (0.95, 0.95, 0.95, 1.0)
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

		float _tex_size;
		float _x_size;
		float _y_size;
		float4 _line_color;
		float4 _back_color;

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

		float3 plot(float tmp_x) {
				float x = tmp_x * _x_size;
				// 这里写计算公式
				float y = sin(x);
				// 公式结束
				float y_tmp = (y/_y_size + 1.0)/2.0;
				return float3(y_tmp, y_tmp, y_tmp);
		}

			float4 frag(v2f i) : COLOR {
				float x = i.uv.x;
				float y = i.uv.y;

				float pixelWidth = 1.0 / _tex_size;
				float pixelHeight = 1.0 / _tex_size;

				int samples = 4;
				float2 step = float2(pixelWidth / samples * 2.0, pixelHeight / samples * 2.0);
				float sum = 0.0;
				for (int i = 0.0; i < samples; i++) {
					for (int j = 0.0; j < samples; j++) {
						float f = plot(x + i * step.x).rgb - (y + j * step.y);
						sum += (f > 0.) ? 1 : -1;
					}
				}

				float alpha = 1.0 - abs(sum) / (samples*samples);
				float3 color = lerp(_back_color, _line_color.rgb, saturate(alpha));
				return float4(color, 1.0);
			}
		ENDCG
		}
	}
	FallBack Off
}

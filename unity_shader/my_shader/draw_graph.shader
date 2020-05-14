Shader "my_test_shader/draw_graph"
{
	Properties{
		_TextureWidth("_TextureWidth", Float) = 512.0
		_TextureHeight("_TextureHeight", Float) = 512.0
		_line_color("_line_color", Color) = (1.0, 0.0, 0.0, 1.0)
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
					#pragma glsl

		// only compile for traditional desktop renderers to avoid strange warnings
		#pragma only_renderers d3d9 d3d11 opengl glcore
		#include "UnityCG.cginc"

		//#include "./ComputeLookups.cginc"
		float _TextureWidth;
		float _TextureHeight;
		float4 _line_color;

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

		float3 plot(float x) {
				float r = x;
				float3 profile = r * r;
				return profile;
		}

			float4 frag(v2f i) : COLOR {
				float aspectRatio = (float)_TextureWidth / (float)_TextureHeight;
				float x = i.uv.x;
				float y = i.uv.y;

				float pixelWidth = 1.0 / _TextureWidth;
				float pixelHeight = 1.0 / _TextureHeight;

				float strokeWidth = max(pixelWidth, pixelHeight);

				float2 p = float2(x, y);

				int samples = 4;
				float2 step = float2(pixelWidth / samples * 2.0, pixelHeight / samples * 2.0);
				float sum = 0.0;
				for (int i = 0.0; i < samples; i++) {
					for (int j = 0.0; j < samples; j++) {
						float f = plot(p.x + i * step.x).rgb - (p.y + j * step.y);
						sum += (f > 0.) ? 1 : -1;
					}
				}

				float3 back_color = float3(0.95, 0.95, 0.95);
				float alpha = 1.0 - abs(sum) / (samples*samples);
				float3 color = lerp(back_color, _line_color.rgb, saturate(alpha));

				//color = float3(y, y, y);
				return float4(color, 1.0);
			}
		ENDCG
		}
	}
	FallBack Off
}

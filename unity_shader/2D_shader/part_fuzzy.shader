Shader "my_shader/part_fuzzy"
{
	Properties
	{
		_MainTex ("Texture", 2D) = "white" {}

		_blur_radius("Blur Radius", Range(2, 15)) = 2
		_mask_tex("Mask Texture", 2D) = "white" {}
		_blur_range("Blur Range,start and wh", Vector) = (0.0, 0.0, 0.0, 0.0)
		_tex_size("Tex Size", Vector) = (100, 100, 1, 0)
		_need_reverse("Need Reverse", Range(0, 1)) = 1
		_detal("Detal", Float) = 1.0
	}
	SubShader
	{
		Tags
		{
			"Queue" = "Transparent"
			"IgnoreProjector" = "True"
			"RenderType" = "Transparent"
			"PreviewType" = "Plane"
			"CanUseSpriteAtlas" = "True"
		}

		Cull Off
		Lighting Off
		ZWrite Off
		Blend SrcAlpha OneMinusSrcAlpha

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

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;
				return o;
			}
			
			sampler2D _MainTex;
			int _blur_radius;
			float4 _blur_range;
			sampler2D _mask_tex;
			float4 _tex_size;
			int _need_reverse;
			float _detal;

			float GetGaussWeight(float x, float y, float sigma)
			{
				float sigma2 = pow(sigma, 2.0f);
				float left = 1 / (2 * sigma2 * 3.1415926f);
				float right = exp(-(x*x + y*y) / (2 * sigma2));	
				return left * right;
			}

			fixed4 GaussBlur(float2 uv)	
			{
				float line_size = 3.0f; // _blur_radius * 2 + 1;
				float sigma = (float)_blur_radius / line_size;
				float4 col = float4(0, 0, 0, 0);
				for (int x = -_blur_radius; x <= _blur_radius; ++x)
				{
					for (int y = -_blur_radius; y <= _blur_radius; ++y)
					{
						float4 color = tex2D(_MainTex, uv + float2(x / _tex_size.x, y / _tex_size.y)*_tex_size.z);
						float weight = GetGaussWeight(x, y, sigma);
						col += color * weight;
					}
				}
				return col;
			}

			fixed4 SimpleBlur(float2 uv)
			{
				float4 col = float4(0, 0, 0, 0);
				for (int x = -_blur_radius; x <= _blur_radius; ++x)
				{
					for (int y = -_blur_radius; y <= _blur_radius; ++y)
					{
						float4 color = tex2D(_MainTex, uv + float2(x / _tex_size.x, y / _tex_size.y));
						//简单的进行颜色累加
						col += color;
					}
				}
				//取平均数，所取像素为边长为(半径*2+1)的矩阵
				col = col / pow(_blur_radius * 2 + 1, 2.0f);
				return col;
			}

			float4 frag (v2f i) : SV_Target
			{
				float4 main_color = tex2D(_MainTex, i.uv);
				float alpha = main_color.a;
				//float4 fuzzy_color = GaussBlur(i.uv); 
				float4 fuzzy_color = SimpleBlur(i.uv);
				
				float2 diff_pos = i.uv - _blur_range.xy;

				float4 black_color = float4(0.0f, 0.0f, 0.0f, 0.0f);
				float4 white_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
				float4 lerp_value = float4(_need_reverse, _need_reverse, _need_reverse, _need_reverse);

				float mask_value = tex2D(_mask_tex, i.uv).r;
				lerp_value = float4(mask_value, mask_value, mask_value, mask_value);

				float4 lerp_color = lerp(black_color , white_color, lerp_value);
				if (diff_pos.x > 0.0f && diff_pos.x < _blur_range.z && diff_pos.y > 0.0f && diff_pos.y < _blur_range.w)
					lerp_color = lerp(white_color, black_color, lerp_value);

				float4 out_col = fuzzy_color; // lerp(main_color, fuzzy_color, lerp_color);
				return out_col;
			}
			ENDCG
		}
	}
}

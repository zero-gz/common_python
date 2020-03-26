Shader "my_shader/2d_light"
{
	Properties
	{
		_MainTex ("Base Texture", 2D) = "black" {}
		_normal_tex("Normal Texture", 2D) = "bump" {}
		_bump_scale("Bump Scale", Range(-5, 5)) = 1.0

		_center ("Light Center", Vector) = (0.5, 0.5, 0.0, 0.0)
		_light_color ("Light Color", Color) = (1.0, 1.0, 1.0, 1.0)
		_light_radius ("Light Radius", Range(0.0, 10.0)) = 1.0
		_falloff ("Light Falloff", Range(0.0, 100.0) ) = 1.0
		_light_strength ("Light Strength", Range(0.0, 10.0)) = 1.0

		_light_dir("Light Dir", Vector) = (0.0, 0.0, 1.0, 0.0)
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
			sampler2D _normal_tex;
			float4 _normal_tex_ST;
			float _bump_scale;

			float4 _center;
			float4 _light_color;
			float _light_radius;
			float _falloff;
			float _light_strength;

			float4 _light_dir;
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = TRANSFORM_TEX(v.uv, _MainTex);
				return o;
			}

			// 第一种点光衰减方式，不要radius, x,y,z是三个衰减项
			/*
			float3 apply_point_light(float2 uv)
			{
				float2 light_center = _center.xy;
				float2 diff = uv - light_center;
				float dist = max(length(diff), 0.0001f);

				float3 out_color = _light_color.rgb * _light_strength / (_falloff.x*_falloff.x*dist + _falloff.y*dist + _falloff.z);
				return out_color;
			}
			*/

			// 第二种点光衰减方式，需要radius, falloff因子
			float3 apply_point_light(float2 uv)
			{
				float2 light_center = _center.xy;
				float2 diff = (uv - light_center)/_light_radius;
				float b = saturate(1.0f - dot(diff, diff));
				float light_atten = pow(b, _falloff+0.0001f);
				float3 out_color = _light_color.rgb * _light_strength * light_atten;
				return out_color;
			}
			
			float4 frag (v2f i) : SV_Target
			{
				float3 normal = UnpackNormal(tex2D(_normal_tex, i.uv));
				normal.xy = normal.xy * _bump_scale;
				normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));


				float3 L = normalize(_light_dir.xyz);
				float NOL = saturate(dot(normal, L));
				//NOL = normal.z;
				// sample the texture
				float4 albedo = tex2D(_MainTex, i.uv);
				
				float3 light_color = apply_point_light(i.uv);
				float3 light_diffuse = light_color*albedo.rgb*NOL;

				float4 final_color = float4(light_diffuse, albedo.a);

				return final_color;
			}
			ENDCG
		}
	}
}

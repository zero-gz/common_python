Shader "my_shader/move_fuzzy"
{
	Properties
	{
		_MainTex("Texture", 2D) = "white" {}
		_move_x("Move X speed", Float) = 0.0
		_move_y("Move Y speed", Float) = 0.0

		_blur_size("Blur Size", Float) = 0.1
		_fuzzy_dir("Fuzzy Dir", Vector) = (1, 0, 0, 0)
		_sample_count("Sample Count", Range(1, 10)) = 5
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

	float _move_x;
	float _move_y;

	float _blur_size;
	float4 _fuzzy_dir;
	float _sample_count;

	#define MID_FUZZY_AVG 1.0/9.0
	// do some fuzzy
	float3 apply_mid_fuzzy(float2 uv, sampler2D tex)
	{
		float3 color_out = float3(0.0, 0.0, 0.0);
		for (int i = -1; i <= 1; i++)
			for (int j = -1; j <= 1; j++)
			{
				float2 aim_uv = uv + float2(i, j)*_blur_size;
				color_out += tex2D(tex, aim_uv).rgb;
			}

		color_out = color_out / 9.0;
		return color_out;
	}

	float4 apply_dir_fuzzy(float2 uv, float2 dir, int sample_count, sampler2D tex)
	{
		float adjust_size[10] = { 1,1,2,2,2,4,4,4,4,4 };
		dir = normalize(dir);
		float4 color_tex = tex2D(tex, uv);
		float3 color_out = color_tex.rgb;

		for (int i = 1; i < sample_count; i++)
		{
			float2 aim_uv = uv + dir*i*_blur_size; // *adjust_size[i];
			color_out += tex2D(tex, aim_uv).rgb;
		}

		color_out = color_out / sample_count;

		return float4(color_out, color_tex.a);
	}

	// can do color grading process

	v2f vert(appdata v)
	{
		v2f o;
		o.vertex = UnityObjectToClipPos(v.vertex);
		o.uv = TRANSFORM_TEX(v.uv, _MainTex);

		o.uv = o.uv * float2(-1, -1);
		return o;
	}

	float2 apply_uv_trans(float2 uv)
	{
		float2 new_uv;
		new_uv = uv + float2(_move_x, _move_y)*_Time.y;
		return new_uv;
	}

	float4 frag(v2f i) : SV_Target
	{
		float2 uv = apply_uv_trans(i.uv);
		float4 out_col = apply_dir_fuzzy(uv, _fuzzy_dir.xy, _sample_count, _MainTex);
		return out_col;
	}
		ENDCG
	}
	}
}

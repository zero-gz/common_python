// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Copyright (c) <2015> <Playdead>
// This file is subject to the MIT License as seen in the root of this folder structure (LICENSE.TXT)
// AUTHOR: Lasse Jon Fuglsang Pedersen <lasse@playdead.com>

Shader "my_shader/test_taa"
{
	Properties
	{
		_MainTex("Base (RGB)", 2D) = "white" {}
	}

		CGINCLUDE
		//--- program begin

#pragma only_renderers ps4 xboxone d3d11 d3d9 xbox360 opengl glcore gles3 metal vulkan
#pragma target 3.0

#pragma multi_compile __ USE_MOTION_BLUR

#pragma enable_d3d11_debug_symbols

#include "UnityCG.cginc"
#include "IncDepth.cginc"
#include "IncNoise.cginc"

#if SHADER_API_MOBILE
		static const float FLT_EPS = 0.0001f;
#else
		static const float FLT_EPS = 0.00000001f;
#endif

	uniform float4 _JitterUV;// frustum jitter uv deltas, where xy = current frame, zw = previous

	uniform sampler2D _MainTex;
	uniform float4 _MainTex_TexelSize;


	sampler2D _VelocityBuffer;
	//sampler2D _CameraMotionVectorsTexture;

	uniform sampler2D _PrevTex;
	uniform float _FeedbackMin;
	uniform float _FeedbackMax;

	struct v2f
	{
		float4 cs_pos : SV_POSITION;
		float2 ss_txc : TEXCOORD0;
	};

	v2f vert(appdata_img IN)
	{
		v2f OUT;

		OUT.cs_pos = UnityObjectToClipPos(IN.vertex);
		OUT.ss_txc = IN.texcoord.xy;

		return OUT;
	}

	// https://software.intel.com/en-us/node/503873
	float3 RGB_YCoCg(float3 c)
	{
		// Y = R/4 + G/2 + B/4
		// Co = R/2 - B/2
		// Cg = -R/4 + G/2 - B/4
		return float3(
			c.x / 4.0 + c.y / 2.0 + c.z / 4.0,
			c.x / 2.0 - c.z / 2.0,
			-c.x / 4.0 + c.y / 2.0 - c.z / 4.0
			);
	}

	// https://software.intel.com/en-us/node/503873
	float3 YCoCg_RGB(float3 c)
	{
		// R = Y + Co - Cg
		// G = Y + Cg
		// B = Y - Co - Cg
		return saturate(float3(
			c.x + c.y - c.z,
			c.x + c.z,
			c.x - c.y - c.z
			));
	}

	float4 sample_color(sampler2D tex, float2 uv)
	{
		float4 c = tex2D(tex, uv);
		return float4(RGB_YCoCg(c.rgb), c.a);
	}

	float4 resolve_color(float4 c)
	{
		return float4(YCoCg_RGB(c.rgb).rgb, c.a);
	}

	float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
	{
		// note: only clips towards aabb center (but fast!)
		float3 p_clip = 0.5 * (aabb_max + aabb_min);
		float3 e_clip = 0.5 * (aabb_max - aabb_min) + FLT_EPS;

		float4 v_clip = q - float4(p_clip, p.w);
		float3 v_unit = v_clip.xyz / e_clip;
		float3 a_unit = abs(v_unit);
		float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

		if (ma_unit > 1.0)
			return float4(p_clip, p.w) + v_clip / ma_unit;
		else
			return q;// point inside aabb
	}

	float2 sample_velocity_dilated(sampler2D tex, float2 uv, int support)
	{
		float2 du = float2(_MainTex_TexelSize.x, 0.0);
		float2 dv = float2(0.0, _MainTex_TexelSize.y);
		float2 mv = 0.0;
		float rmv = 0.0;

		int end = support + 1;
		for (int i = -support; i != end; i++)
		{
			for (int j = -support; j != end; j++)
			{
				float2 v = tex2D(tex, uv + i * dv + j * du).xy;
				float rv = dot(v, v);
				if (rv > rmv)
				{
					mv = v;
					rmv = rv;
				}
			}
		}

		return mv;
	}

	float4 sample_color_motion(sampler2D tex, float2 uv, float2 ss_vel)
	{
		const float2 v = 0.5 * ss_vel;
		const int taps = 3;// on either side!

		float srand = PDsrand(uv + _SinTime.xx);
		float2 vtap = v / taps;
		float2 pos0 = uv + vtap * (0.5 * srand);
		float4 accu = 0.0;
		float wsum = 0.0;

		[unroll]
		for (int i = -taps; i <= taps; i++)
		{
			float w = 1.0;// box
			//float w = taps - abs(i) + 1;// triangle
			//float w = 1.0 / (1 + abs(i));// pointy triangle
			accu += w * sample_color(tex, pos0 + i * vtap);
			wsum += w;
		}

		return accu / wsum;
	}

	float4 temporal_reprojection(float2 ss_txc, float2 ss_vel, float vs_dist)
	{
		// read texels
		float4 texel0 = sample_color(_MainTex, ss_txc - _JitterUV.xy);
		float4 texel1 = sample_color(_PrevTex, ss_txc - ss_vel);

		// calc min-max of current neighbourhood
		float2 uv = ss_txc;

		float2 du = float2(_MainTex_TexelSize.x, 0.0);
		float2 dv = float2(0.0, _MainTex_TexelSize.y);

		float4 ctl = sample_color(_MainTex, uv - dv - du);
		float4 ctc = sample_color(_MainTex, uv - dv);
		float4 ctr = sample_color(_MainTex, uv - dv + du);
		float4 cml = sample_color(_MainTex, uv - du);
		float4 cmc = sample_color(_MainTex, uv);
		float4 cmr = sample_color(_MainTex, uv + du);
		float4 cbl = sample_color(_MainTex, uv + dv - du);
		float4 cbc = sample_color(_MainTex, uv + dv);
		float4 cbr = sample_color(_MainTex, uv + dv + du);

		float4 cmin = min(ctl, min(ctc, min(ctr, min(cml, min(cmc, min(cmr, min(cbl, min(cbc, cbr))))))));
		float4 cmax = max(ctl, max(ctc, max(ctr, max(cml, max(cmc, max(cmr, max(cbl, max(cbc, cbr))))))));

		float4 cavg = (ctl + ctc + ctr + cml + cmc + cmr + cbl + cbc + cbr) / 9.0;

		// shrink chroma min-max
		float2 chroma_extent = 0.25 * 0.5 * (cmax.r - cmin.r);
		float2 chroma_center = texel0.gb;
		cmin.yz = chroma_center - chroma_extent;
		cmax.yz = chroma_center + chroma_extent;
		cavg.yz = chroma_center;

		// clamp to neighbourhood of current sample
		texel1 = clip_aabb(cmin.xyz, cmax.xyz, clamp(cavg, cmin, cmax), texel1);

		// feedback weight from unbiased luminance diff (t.lottes)
		float lum0 = texel0.r;
		float lum1 = texel1.r;
		float unbiased_diff = abs(lum0 - lum1) / max(lum0, max(lum1, 0.2));
		float unbiased_weight = 1.0 - unbiased_diff;
		float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
		float k_feedback = lerp(_FeedbackMin, _FeedbackMax, unbiased_weight_sqr);

		// output
		return lerp(texel0, texel1, k_feedback);
	}

	//f2rt frag(v2f IN)
	float4 frag(v2f IN) : SV_TARGET
	{
		float2 uv = IN.ss_txc;

		//--- 3x3 nearest (good)
		float3 c_frag = find_closest_fragment_3x3(uv);
		float2 ss_vel = tex2D(_VelocityBuffer, c_frag.xy).xy;
		float vs_dist = LinearEyeDepth(c_frag.z);

		// temporal resolve
		float4 color_temporal = temporal_reprojection(IN.ss_txc, ss_vel, vs_dist);

		// prepare outputs
		float4 to_buffer = resolve_color(color_temporal);

	#if USE_MOTION_BLUR
		float vel_mag = length(ss_vel * _MainTex_TexelSize.zw);
		const float vel_trust_full = 2.0;
		const float vel_trust_none = 15.0;
		const float vel_trust_span = vel_trust_none - vel_trust_full;
		float trust = 1.0 - clamp(vel_mag - vel_trust_full, 0.0, vel_trust_span) / vel_trust_span;

		float4 color_motion = sample_color_motion(_MainTex, IN.ss_txc - _JitterUV.xy, ss_vel);

		float4 to_screen = resolve_color(lerp(color_motion, color_temporal, trust));
	#else
		float4 to_screen = resolve_color(color_temporal);
	#endif

		// add noise
		float4 noise4 = PDsrand4(IN.ss_txc + _SinTime.x + 0.6959174) / 510.0;
		return saturate(to_buffer + noise4);
	}

		//--- program end
		ENDCG

		SubShader
	{
		ZTest Always Cull Off ZWrite Off
			Fog{ Mode off }

			Pass
		{
			CGPROGRAM

			#pragma vertex vert
			#pragma fragment frag

			ENDCG
		}
	}

	Fallback off
}
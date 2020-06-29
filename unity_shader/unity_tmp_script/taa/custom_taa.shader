Shader "my_shader/custom_taa"
{
	Properties
	{
		_MainTex("Texture", 2D) = "white" {}
	}
		SubShader
		{
			Tags { "RenderType" = "Opaque" }
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
				};

				sampler2D _MainTex;
				float4 _MainTex_ST;
				float4 _MainTex_TexelSize;

				sampler2D _CameraMotionVectorsTexture;
				sampler2D _CameraDepthTexture;
				float4 _CameraDepthTexture_TexelSize;
				static const float FLT_EPS = 0.00000001f;

				uniform float4 _JitterUV;

				uniform sampler2D _PrevTex;
				uniform float _FeedbackMin;
				uniform float _FeedbackMax;

				v2f vert(appdata v)
				{
					v2f o;
					o.vertex = UnityObjectToClipPos(v.vertex);
					o.uv = TRANSFORM_TEX(v.uv, _MainTex);
					return o;
				}

				struct f2rt
				{
					fixed4 buffer : SV_Target0;
					fixed4 screen : SV_Target1;
				};

#if UNITY_REVERSED_Z
#define ZCMP_GT(a, b) (a < b)
#else
#define ZCMP_GT(a, b) (a > b)
#endif

				float depth_sample_linear(float2 uv)
				{
					return LinearEyeDepth(tex2D(_CameraDepthTexture, uv).x);
				}

				float3 find_closest_fragment_3x3(float2 uv)
				{
					float2 dd = abs(_CameraDepthTexture_TexelSize.xy);
					float2 du = float2(dd.x, 0.0);
					float2 dv = float2(0.0, dd.y);

					float3 dtl = float3(-1, -1, tex2D(_CameraDepthTexture, uv - dv - du).x);
					float3 dtc = float3(0, -1, tex2D(_CameraDepthTexture, uv - dv).x);
					float3 dtr = float3(1, -1, tex2D(_CameraDepthTexture, uv - dv + du).x);

					float3 dml = float3(-1, 0, tex2D(_CameraDepthTexture, uv - du).x);
					float3 dmc = float3(0, 0, tex2D(_CameraDepthTexture, uv).x);
					float3 dmr = float3(1, 0, tex2D(_CameraDepthTexture, uv + du).x);

					float3 dbl = float3(-1, 1, tex2D(_CameraDepthTexture, uv + dv - du).x);
					float3 dbc = float3(0, 1, tex2D(_CameraDepthTexture, uv + dv).x);
					float3 dbr = float3(1, 1, tex2D(_CameraDepthTexture, uv + dv + du).x);

					float3 dmin = dtl;
					if (ZCMP_GT(dmin.z, dtc.z)) dmin = dtc;
					if (ZCMP_GT(dmin.z, dtr.z)) dmin = dtr;

					if (ZCMP_GT(dmin.z, dml.z)) dmin = dml;
					if (ZCMP_GT(dmin.z, dmc.z)) dmin = dmc;
					if (ZCMP_GT(dmin.z, dmr.z)) dmin = dmr;

					if (ZCMP_GT(dmin.z, dbl.z)) dmin = dbl;
					if (ZCMP_GT(dmin.z, dbc.z)) dmin = dbc;
					if (ZCMP_GT(dmin.z, dbr.z)) dmin = dbr;

					return float3(uv + dd.xy * dmin.xy, dmin.z);
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

				float4 PDnrand4(float2 n) {
					return frac(sin(dot(n.xy, float2(12.9898f, 78.233f)))* float4(43758.5453f, 28001.8384f, 50849.4141f, 12996.89f));
				}

				float4 PDsrand4(float2 n) {
					return PDnrand4(n) * 2 - 1;
				}

				float4 temporal_reprojection(float2 ss_txc, float2 ss_vel, float vs_dist)
				{
					// change to ycocg

					// read texels
					float4 texel0 = sample_color(_MainTex, ss_txc - _JitterUV.xy);
					float4 texel1 = sample_color(_PrevTex, ss_txc - ss_vel);

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

					// 这段是搞不懂的
					float2 chroma_extent = 0.25 * 0.5 * (cmax.r - cmin.r);
					float2 chroma_center = texel0.gb;
					cmin.yz = chroma_center - chroma_extent;
					cmax.yz = chroma_center + chroma_extent;
					cavg.yz = chroma_center;

					// clamp to neighbourhood of current sample
					texel1 = clip_aabb(cmin.xyz, cmax.xyz, clamp(cavg, cmin, cmax), texel1);

					// feedback weight from unbiased luminance diff (t.lottes)
					float lum0 = Luminance(texel0.rgb);
					float lum1 = Luminance(texel1.rgb);

					float unbiased_diff = abs(lum0 - lum1) / max(lum0, max(lum1, 0.2));
					float unbiased_weight = 1.0 - unbiased_diff;
					float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
					float k_feedback = lerp(_FeedbackMin, _FeedbackMax, unbiased_weight_sqr);

					// output
					return lerp(texel0, texel1, k_feedback);
				}

				fixed4 frag(v2f i) : SV_Target
				{
					f2rt OUT;
					float2 uv = i.uv;

					//--- 3x3 nearest (good)
					float3 c_frag = find_closest_fragment_3x3(uv);
					float2 ss_vel = tex2D(_CameraMotionVectorsTexture, c_frag.xy).xy;
					float vs_dist = depth_sample_linear(c_frag.z);

					// temporal resolve
					float4 color_temporal = temporal_reprojection(uv, ss_vel, vs_dist);

					// add noise
					float4 noise4 = PDsrand4(uv + _SinTime.x + 0.6959174) / 510.0;
					float4 output = saturate(color_temporal + noise4);
						
					return output;
				}
				ENDCG
			}
		}
}

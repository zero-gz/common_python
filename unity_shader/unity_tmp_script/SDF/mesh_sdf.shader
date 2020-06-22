Shader "Custom/mesh_sdf"
{
	Properties
	{
		_MainTex("Texture", 2D) = "white" {}
	}

		SubShader
	{
		// No culling or depth
		Cull Off ZWrite Off ZTest Always

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag

			#include "UnityCG.cginc"

			float4 _MainTex_TexelSize;

			float3 _CamPosition;
			float4x4 _PixelCoordToViewDirWS;
			//传入光的方向
			float3 _LightDir;
			float3 _LightPos;

			float4 _Sphere1;
			float4 _Sphere2;
			float4 _box;
			float4 _plane;

			struct SDFrVolumeData
			{
				float4x4 WorldToLocal;
				float3 Extents;
			};
			
			StructuredBuffer<SDFrVolumeData> _VolumeBuffer;
			Texture3D _VolumeATex;
			SamplerState sdfr_sampler_linear_clamp;
#define NORMAL_DELTA 0.03

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

			float map(float3 p);

			//#定义有向距离场方程, C就是圆心位置，r是指半径
			float sdSphere(float3 rp, float3 c, float r)
			{
				return distance(rp,c) - r;
			}

			//p是目标点－长方体中心位置，b是半长，这个r有什么用？
			float sdCube(float3 p, float3 b, float r)
			{
			  return length(max(abs(p) - b,0.0)) - r;
			}

			float sdPlane(float3 p, float y_offset)
			{
				return p.y + y_offset;
			}

			
			inline float2 LineAABBIntersect(float3 ro, float3 re, float3 aabbMin, float3 aabbMax)
			{
				float3 invRd = 1.0 / (re - ro);
				float3 mini = invRd * (aabbMin - ro);
				float3 maxi = invRd * (aabbMax - ro);
				float3 closest = min(mini, maxi);
				float3 furthest = max(mini, maxi);

				float2 intersections;
				//furthest near intersection
				intersections.x = max(closest.x, max(closest.y, closest.z));
				//closest far intersection
				intersections.y = min(furthest.x, min(furthest.y, furthest.z));
				//map 0 ray origin, 1 ray end
				return saturate(intersections);
			}

			//incoming ray is world space
			inline float DistanceFunctionTex3D(in float3 rayPosWS, in float3 rayOrigin, in float3 rayEnd, in SDFrVolumeData data, in Texture3D tex)
			{
				float4x4 w2l = data.WorldToLocal;
				float3 extents = data.Extents;

				//take ray to local space of bounds (now the bounds is axis-aligned relative to the ray)
				float3 originLocal = mul(w2l, float4(rayOrigin, 1)).xyz;
				float3 endLocal = mul(w2l, float4(rayEnd, 1)).xyz;

				float2 intersection = LineAABBIntersect(originLocal, endLocal, -extents, extents);

				//if the ray intersects 
				if (intersection.x < intersection.y && intersection.x < 1)
				{
					float3 rayPosLocal = mul(w2l, float4(rayPosWS, 1)).xyz;
					rayPosLocal /= extents.xyz * 2;
					rayPosLocal += 0.5; //texture space
					//values are -1 to 1
					float sample = tex.SampleLevel(sdfr_sampler_linear_clamp, rayPosLocal, 0).r;
					//float sample = tex2DLod(tex, rayPosLocal, 0).r;
					sample *= length(extents); //scale by magnitude of bound extent -- NC: Should this by times 2 as extents is halfsize?
					return sample;
				}
				//TODO not really consistent 
				return distance(rayOrigin, rayEnd);
			}

			inline float DistanceFunctionTex3DFast(in float3 rayPosWS, in SDFrVolumeData data, in Texture3D tex)
			{
				float4x4 w2l = data.WorldToLocal;
				float3 extents = data.Extents;

				float3 rayPosLocal = mul(w2l, float4(rayPosWS, 1)).xyz;
				rayPosLocal /= extents.xyz * 2;
				rayPosLocal += 0.5; //texture space
				//values are -1 to 1
				float sample = tex.SampleLevel(sdfr_sampler_linear_clamp, rayPosLocal, 0).r;
				sample *= length(extents); //scale by magnitude of bound extent
				return sample;
			}
			
			// Does no boundss checking - so assuming we hit something - but still checking against all primitives
			float DistanceFunctionFast(float3 rayPos)
			{
				float sp = sdSphere(rayPos, _Sphere1.xyz, _Sphere1.w);
				float sp2 = sdSphere(rayPos, _Sphere2.xyz, _Sphere2.w);
				float cb = sdCube(rayPos - _box.xyz, _box.w*float3(1.0, 1.0, 1.0), 0.0);

				float d = min(min(sp, sp2), cb);

				float ad = DistanceFunctionTex3DFast(rayPos, _VolumeBuffer[0], _VolumeATex);
				d = min(ad, d);

				//float plane_distance = sdPlane(rayPos, 3);
				//d = min(d, plane_distance);
				return d;
			}

			float map(float3 rp)
			{
				float ret;
				float sp = sdSphere(rp, _Sphere1.xyz, _Sphere1.w);
				float sp2 = sdSphere(rp, _Sphere2.xyz, _Sphere2.w);
				float cb = sdCube(rp - _box.xyz, _box.w*float3(1.0,1.0, 1.0), 0.0);
				float py = sdPlane(rp.y, _plane.w);
				ret = (sp < py) ? sp : py;
				ret = (ret < sp2) ? ret : sp2;
				ret = (ret < cb) ? ret : cb;
				return ret;
			}


			//Adapted from:iquilezles
			float calcSoftshadow(float3 ro, float3 rd, in float mint, in float tmax)
			{

				float res = 1.0;
				float t = mint;
				float ph = 1e10;

				for (int i = 0; i < 32; i++)
				{
					float h = map(ro + rd * t);
					float y = h * h / (2.0*ph);
					float d = sqrt(h*h - y * y);
					res = min(res, 10.0*d / max(0.0,t - y));
					ph = h;

					t += h;

					if (res<0.0001 || t>tmax)
						break;

				}
				return clamp(res, 0.0, 1.0);
			}

			float3 calcNorm(float3 p);

			//实现raymarching的逻辑
			//raymarching的逻辑在fragment shader中来实现，代表沿着射向该像素的射线进行采样。
			//所以输入要指明该射线的其实位置，以及它的方向。
			//返回则是一个颜色值
			
			fixed4 raymarching(float3 rayOrigin, float3 rayDirection)
			{

				fixed4 ret = fixed4(0, 0, 0, 0);

				int maxStep = 64;

				float rayDistance = 0;

				for (int i = 0; i < maxStep; i++)
				{
					float3 p = rayOrigin + rayDirection * rayDistance;
					float surfaceDistance = map(p);
					//float mesh_distance = DistanceFunctionTex3D(p, rayOrigin, rayOrigin + rayDirection * 10000, _VolumeBuffer[0], _VolumeATex);
					//surfaceDistance = min(surfaceDistance, mesh_distance);

					if (surfaceDistance < 0.001)
					{
						ret = fixed4(1, 0, 0, 1);
						//增加光照效果
						/*
						float halfDelta = NORMAL_DELTA * 0.5;
						float dx = DistanceFunctionFast(p + float3(halfDelta, 0, 0)) - DistanceFunctionFast(p - float3(halfDelta, 0, 0));
						float dy = DistanceFunctionFast(p + float3(0, halfDelta, 0)) - DistanceFunctionFast(p - float3(0, halfDelta, 0));
						float dz = DistanceFunctionFast(p + float3(0, 0, halfDelta)) - DistanceFunctionFast(p - float3(0, 0, halfDelta));

						float3 normalWS = normalize(float3(dx, dy, dz));
						*/
						float3 norm = calcNorm(p);
						ret = clamp(dot(-_LightDir, norm), 0, 1) *calcSoftshadow(p, -_LightDir, 0.01, 300.0);
						ret.a = 1;
						break;
					}

					rayDistance += surfaceDistance;
				}
				return ret;
			}
			
			/*
			fixed4 raymarching(float3 rayOrigin, float3 rayDirection)
			{

				fixed4 ret = fixed4(0, 0, 0, 0);

				int maxStep = 64;

				float rayDistance = 0;

				for (int i = 0; i < maxStep; i++)
				{
					float3 p = rayOrigin + rayDirection * rayDistance;
					float surfaceDistance = map(p);
					if (surfaceDistance < 0.001)
					{
						ret = fixed4(1, 0, 0, 1);
						break;
					}

					rayDistance += surfaceDistance;
				}
				return ret;
			}
			*/


			//计算光照
			//计算法线
			float3 calcNorm(float3 p)
			{
				float eps = 0.001;

				float3 norm = float3(
					map(p + float3(eps, 0, 0)) - map(p - float3(eps, 0, 0)),
					map(p + float3(0, eps, 0)) - map(p - float3(0, eps, 0)),
					map(p + float3(0, 0, eps)) - map(p - float3(0, 0, eps))
				);

				return normalize(norm);
			}


			v2f vert(appdata v)
			{
				v2f o;

				fixed index = v.vertex.z;

				v.vertex.z = 0;

				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;
				return o;
			}


			sampler2D _MainTex;

			fixed4 frag(v2f i) : SV_Target
			{
				float3 rayOrigin = _CamPosition;
				float3 rayDirection = -normalize(mul(float3(i.vertex.xy, 1.0), (float3x3)_PixelCoordToViewDirWS));
				float3 rayEnd = rayOrigin + rayDirection * _ProjectionParams.z;

				fixed4 bgColor = tex2D(_MainTex, i.uv);

				fixed4 objColor = raymarching(rayOrigin, rayDirection);

				fixed4 finalColor = fixed4(objColor.xyz * objColor.w + bgColor.xyz * (1 - objColor.w), 1);
				return finalColor;
			}
			ENDCG
		}
	}
}
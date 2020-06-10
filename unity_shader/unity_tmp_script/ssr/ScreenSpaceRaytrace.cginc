#ifndef _YRC_SCREEN_SPACE_RAYTRACE_
#define _YRC_SCREEN_SPACE_RAYTRACE_

#define RAY_LENGTH 40.0	//maximum ray length.
#define STEP_COUNT 16	//maximum sample count.
#define PIXEL_STRIDE 16 //sample multiplier. it's recommend 16 or 8.
#define PIXEL_THICKNESS (0.03 * PIXEL_STRIDE)	//how thick is a pixel. correct value reduces noise.

bool RayIntersect(float raya, float rayb, float2 sspt) {
	if (raya > rayb) {
		float t = raya;
		raya = rayb;
		rayb = t;
	}

#if 1		//by default we use fixed thickness.
	float screenPCameraDepth = -LinearEyeDepth(tex2Dlod(_CameraDepthTexture, float4(sspt / 2 + 0.5, 0, 0)).r);
	return raya < screenPCameraDepth && rayb > screenPCameraDepth - PIXEL_THICKNESS;
#else
	//float backZ = tex2Dlod(_BackfaceTex, float4(sspt / 2 + 0.5, 0, 0)).r;
	//return raya < backZ && rayb > screenPCameraDepth;
#endif
}

// start,direction已是 相机空间的点和方向了，不是世界空间的
bool traceRay(float3 start, float3 direction, float jitter, float4 texelSize, out float2 hitPixel, out float marchPercent,out float hitZ) {
	//clamp raylength to near clip plane.
	float rayLength = ((start.z + direction.z * RAY_LENGTH) > -_ProjectionParams.y) ?
		(-_ProjectionParams.y - start.z) / direction.z : RAY_LENGTH; // Z方向上搜索深度 最多40单位长度

	float3 end = start + direction * rayLength;

	float4 H0 = mul(unity_CameraProjection, float4(start, 1)); //这里是乘以相机的投影矩阵
	float4 H1 = mul(unity_CameraProjection, float4(end, 1));

	float2 screenP0 = H0.xy / H0.w;
	float2 screenP1 = H1.xy / H1.w;	

	float k0 = 1.0 / H0.w;
	float k1 = 1.0 / H1.w;

	float Q0 = start.z * k0;
	float Q1 = end.z * k1;

	if (abs(dot(screenP1 - screenP0, screenP1 - screenP0)) < 0.00001) {
		screenP1 += texelSize.xy;
	}
	float2 deltaPixels = (screenP1 - screenP0) * texelSize.zw;
	float step;	//the sample rate.

	//取的是更长的那一个x或是y的步数的倒数
	step = min(1 / abs(deltaPixels.y), 1 / abs(deltaPixels.x)); //make at least one pixel is sampled every time.

	//make sample faster. 这里乘以16是一个经验值，如果正常情况下，要一格一格的走，现在是16格一步，很快就能够命中，如果命中后，再进行仔细的二分查找，最多5次就搞定了	
	step *= PIXEL_STRIDE;		
	float sampleScaler = 1.0 - min(1.0, -start.z / 100); //sample is slower when far from the screen.
	step *= 1.0 + sampleScaler;	

	float interpolationCounter = step;	//by default we use step instead of 0. this avoids some glitch.

	float4 pqk = float4(screenP0, Q0, k0); // screenP0是屏幕空间坐标，Q0是相机空间的Z/投影空间的w，k0是 投影空间的1/w
	float4 dpqk = float4(screenP1 - screenP0, Q1 - Q0, k1 - k0) * step; //这个是上面四个量每次步进的值

	//这里的jitter最不明白的一点，是因为jitter一般小于1，那么步进步长没那么长，那么应该搜索不到终点呢？ 是不是这并不重要？
	pqk += jitter * dpqk;

	float prevZMaxEstimate = start.z;

	bool intersected = false;
	UNITY_LOOP		//the logic here is a little different from PostProcessing or (casual-effect). but it's all about raymarching.
		for (int i = 1;
			i <= STEP_COUNT && interpolationCounter <= 1 && !intersected;
			i++,
			interpolationCounter += step
			) {
		pqk += dpqk;
		float rayZMin = prevZMaxEstimate; //这里的rayZMin和rayZMax是相机空间的Z值，主要是因为想正确插值，必须将其除以投影空间的W
		float rayZMax = ( pqk.z) / ( pqk.w);

		// 相机空间的Z0,Z1,以及屏幕空间的xy中心点（步长半长）
		if (RayIntersect(rayZMin, rayZMax, pqk.xy - dpqk.xy / 2)) {
			hitPixel = (pqk.xy - dpqk.xy / 2) / 2 + 0.5;
			marchPercent = (float)i / STEP_COUNT;
			intersected = true;
		}
		else {
			prevZMaxEstimate = rayZMax;
		}
	}

#if 1	  //binary search
	if (intersected) {
		pqk -= dpqk;	//one step back
		UNITY_LOOP
			for (float gapSize = PIXEL_STRIDE; gapSize > 1.0; gapSize /= 2) {
				dpqk /= 2;
				float rayZMin = prevZMaxEstimate;
				float rayZMax = (pqk.z) / ( pqk.w);

				if (RayIntersect(rayZMin, rayZMax, pqk.xy - dpqk.xy / 2)) {		//hit, stay the same.(but ray length is halfed)

				}
				else {							//miss the hit. we should step forward
					pqk += dpqk;
					prevZMaxEstimate = rayZMax;
				}
			}
		hitPixel = (pqk.xy - dpqk.xy / 2) / 2 + 0.5;
	}
#endif
	hitZ = pqk.z / pqk.w;

	return intersected;
}

#endif
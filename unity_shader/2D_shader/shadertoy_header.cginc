#ifndef SHADERTOY_HEADER
#define SHADERTOY_HEADER

#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mat2 float2x2
#define mat4 float4x4
#define fract frac
#define iResolution _ScreenParams
// 屏幕中的坐标，以pixel为单位
#define gl_FragCoord ((i.srcPos.xy/i.srcPos.w)*_ScreenParams.xy) 
#define iTime _Time.y
#define mix lerp

struct appdata
{
	float4 vertex : POSITION;
	float2 uv : TEXCOORD0;
};

struct v2f
{
	float2 uv : TEXCOORD0;
	float4 pos : SV_POSITION;
	float4 srcPos : TEXCOORD1;
};

sampler2D _MainTex;
float4 _MainTex_ST;

v2f vert(appdata v)
{
	v2f o;
	o.pos = UnityObjectToClipPos(v.vertex);
	o.uv = TRANSFORM_TEX(v.uv, _MainTex);
	o.srcPos = ComputeScreenPos(o.pos);
	return o;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord);

fixed4 frag(v2f i) : SV_Target
{
	vec4 fragColor = vec4(0.0, 0.0, 0.0, 1.0);
	vec2 fragCoord = gl_FragCoord;
	mainImage(fragColor, fragCoord);
	return fragColor;
}

#endif
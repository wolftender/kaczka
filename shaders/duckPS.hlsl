Texture2D albedo : register(t0);
SamplerState samp : register(s0);

cbuffer cbSurfaceColor : register(b0) {
	float4 surfaceColor;
};

cbuffer cbLights : register(b1) {
	float4 lightPos;
};

struct PSInput {
	float4 pos : SV_POSITION;
	float3 worldPos : POSITION0;
	float3 norm : NORMAL0;
	float3 viewVec : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float kd = 0.5, ks = 0.2f, m = 100.0f;

float4 main (PSInput i) : SV_TARGET {
	float3 color = albedo.Sample (samp, i.uv).xyz;
	return float4 (color, 1.0f);
}
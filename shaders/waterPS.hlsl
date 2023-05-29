Texture2D bumpMap : register(t0);
SamplerState samp : register(s0);

struct PSInput {
	float4 pos : SV_POSITION;
	float3 localPos : POSITION0;
	float3 worldPos : POSITION1;
	float3 viewPos : POSITION2;
	float3 worldNorm : NORMAL0;
	float3 viewVec : TEXCOORD0;
};

float4 main(PSInput i) : SV_TARGET {
	float bump = bumpMap.Sample(samp, i.localPos.xy);
	return float4(bump, bump, bump, 1.0f);
}
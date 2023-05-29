Texture2D<float> gPrevTexture : register(t0);
RWTexture2D<float> gOutTexture : register(u0);

[numthreads(16, 16, 1)]
void main (uint3 DTid : SV_DispatchThreadID) {
	float val = gPrevTexture[DTid.xy].r;
	val += 0.01f;

	if (val > 1.0f) {
		val = 0.0f;
	}

	gOutTexture[DTid.xy].r = val;
}
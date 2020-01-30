struct VertInput
{
	uint vertid : SV_VERTEXID;
};

struct VertToPixel
{
	float4 position: SV_POSITION;
};

static float3 POSITIONS[3] = {
	float3(-.5f, -.5f, 0.f),
	float3(-.25f, .5f, 0.f),
	float3(.5f, -.25f, 0.f),
};

VertToPixel Vert(VertInput input)
{
	VertToPixel result = (VertToPixel)0;
	result.position = float4(POSITIONS[input.vertid], 1.f);
	return result;
}

float4 Pixel(VertToPixel input) : SV_Target0
{
	return float4(0.f, 1.f, 1.f, 1.f);
}
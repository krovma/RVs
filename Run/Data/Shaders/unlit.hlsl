struct VertInput
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

cbuffer frame_constants: register(b1)
{
	float TIME;
};

cbuffer camera_constants : register(b2)
{
	//float2 ORTHO_MIN;
	//float2 ORTHO_MAX;
	float4x4 PROJECTION;
	float4x4 VIEW;
	//float4x4 MODEL;
};

cbuffer model_constants : register(b3)
{
	float4x4 MODEL;
};

Texture2D<float4> tAlbedo : register(t0);
SamplerState sAlbedo : register(s0);

struct VertToPixel
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

float FloatMapToNCS(float input, float inputMin, float inputMax)
{

	float outputMin = -1.0f;
	float outputMax = 1.0f;
	float inputRange = inputMax - inputMin;
	return (outputMax - outputMin) * ((input - inputMin) / inputRange) + outputMin;
}

VertToPixel Vert(VertInput input)
{
	VertToPixel result = (VertToPixel)0;

	//float3 inPos = input.position;
	//float clip_x = FloatMapToNCS(inPos.x, ORTHO_MIN.x, ORTHO_MAX.x);
	//float clip_y = FloatMapToNCS(inPos.y, ORTHO_MIN.y, ORTHO_MAX.y);
	//float c = cos(TIME);
	//float s = sin(TIME);
	//result.position = float4(clip_x * c - clip_y * s, clip_y * c + clip_x * s, 0.0f, 1.0f);
	//result.position = float4(clip_x, clip_y, 0, 1);
	
	float4 localPos = float4(input.position, 1.0f);
	float4 worldPos = mul(MODEL, localPos);
	float4 viewPos = mul(VIEW, worldPos);
	float4 clipPos = mul(PROJECTION, viewPos);

	result.position = clipPos;
	result.color = input.color;
	result.uv = input.uv;

	return result;
}

float4 Pixel(VertToPixel input) : SV_TARGET0
{
	float4 texColor = tAlbedo.Sample(sAlbedo, input.uv);
	float4 finalColor = texColor * input.color;
	return finalColor;
}
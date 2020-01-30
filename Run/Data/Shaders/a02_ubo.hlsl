struct VertInput
{
	uint vertid : SV_VERTEXID;
};

struct VertToPixel
{
	float4 position: SV_POSITION;
};

cbuffer CameraConstants : register(b2)
{
	float2 ORTHO_MIN;
	float2 ORTHO_MAX;
};

static float3 SOME_POSITIONS[3] = {
   float3(50.f, 50.f, 0.0f),
   float3(50.f,  100.f, 0.0f),
   float3(100.f, 50.f, 0.0f),
   
/*	float3(-0.5f, 0.5f, 0.0f),
   float3(0.25f, 0.5f, 0.0f),
   float3(0.5f, 0.25f, 0.0f),*/
/*	float3(0.5f, 0.5f, 0.0f),
	  float3(0.25f, 0.5f, 0.0f),
	  float3(0.5f, 0.25f, 0.0f),*/

};

float FloatMapToNCS(float input, float inputMin, float inputMax)
{
	float outputMin = -1.0f;
	float outputMax = 1.0f;
	float inputRange = inputMax - inputMin;
	if (inputRange < 1e-5) {
		return (outputMax + outputMin) / 2.f;
	}
	return (outputMax - outputMin) * ((input - inputMin) / inputRange) + outputMin;
}

VertToPixel Vert(VertInput input)
{
	VertToPixel result = (VertToPixel)0;
	float3 position = SOME_POSITIONS[input.vertid];
	float4 localPos = float4(position, 1.f);
	float clipX = FloatMapToNCS(localPos.x, ORTHO_MIN.x, ORTHO_MAX.x);
	float clipY = FloatMapToNCS(localPos.y, ORTHO_MIN.y, ORTHO_MAX.y);
	float4 clipPos = float4(clipX, clipY, 0.f, 1.f);
	result.position = clipPos;
	return result;
}

float4 Pixel(VertToPixel input) : SV_TARGET0
{
	return float4(1.f, 1.f, 1.f, 1.f);
}
#define MAX_LIGHTS (8)

struct VertInput
{
	float3 position	:POSITION;
	float4 color	:COLOR;
	float2 texUV	:TEXCOORD;
	float3 normal	:NORMAL;
	float3 tangent	:TANGENT;
};

struct VertToPixel
{
	float4 position :SV_POSITION;
	float4 color	:COLOR;
	float2 texUV	:TEXCOORD;
	float3 normal	:NORMAL;
	float3 tangent	:TANGENT;
	float4 worldPos :WORLDPOS;
};

cbuffer frame_constants: register(b1)
{
	float TIME;
	float GAMMA;
	float DRAW_NORMAL;
	float DRAW_TANGENT;
	float DRAW_SPECULAR;
	float EMMISIVE;
	float2 __0__FramePadding;
};

struct Light
{
	float3 color;
	float intensity;

	float3 position;
	float isDirectional;

	float3 direction;
		float __0__LightPadding;

	float3 diffuseAttenuation;
		float __1__LightPadding;

	float3 specularAttenuation;
		float __2__LightPadding;
};

cbuffer CameraConstants : register(b2)
{
	float4x4 PROJECTION;
	float4x4 VIEW;
	float4x4 MPROJ;
	float3 CAMERA_POSITION;
		float __0__CameraPadding;
};

cbuffer ModelConstants : register(b3)
{
	float4x4 MODEL;
};

Texture2D<float4> tAlbedo : register(t0);
SamplerState sAlbedo : register(s0);
Texture2D<float4> tNormal : register(t1);
SamplerState sNormal : register(s1);
Texture2D<float4> tSpecular : register(t3);
SamplerState sSpecular : register(s3);
Texture2D<float4> tEmmisive : register(t2);
SamplerState sEmmisive : register(s2);

cbuffer LightConstants : register(b4)
{
	float4 AMBIENT;
	float SPECULAR_FACTOR;
	float SPECULAR_POWER;
		float2 __0__LightConstantsPadding;
	Light LIGHTS[MAX_LIGHTS];
};

struct Lighting
{
	float3 diffuse;
	float3 specular;
};

Lighting GetLighting(float3 eye, float3 surfacePosition, float3 surfaceNormal, float specularFactor)
{
	Lighting result;
	result.diffuse = AMBIENT.xyz * AMBIENT.w;
	result.specular = float3(0, 0, 0);
	//return result;
	float3 toEyeDir = normalize(eye - surfacePosition);
	for(int i = 0; i < MAX_LIGHTS; ++i){
		Light each = LIGHTS[i];
		//each.direction = 
		float3 directionalDir = each.direction;
		float3 pointDir = normalize(surfacePosition - each.position);
		float3 dir = lerp(pointDir, directionalDir, each.isDirectional);
		//Diffuse
		float directionalDist = abs(dot(surfacePosition - each.position, each.direction));
		float pointDist = length(surfacePosition - each.position);
		float dist = lerp(pointDist, directionalDist, each.isDirectional);
		float3 diffuseAtt = each.diffuseAttenuation;
		float att = 1.0f / (1.0f + diffuseAtt.x + diffuseAtt.y * dist + diffuseAtt.z * dist * dist);
		float dot3 = max(dot(-dir, (surfaceNormal)), 0.0f);
		float3 diffuseColor = each.color * each.intensity * att * dot3;

		/*
		float3 a = diffuseColor;
		float3 b = float3(0, 0, 0);//result.diffuse;
		float3 c = a;//diffuseColor + result.diffuse;
		a = a + b;
		if (a.x != c.x || a.y!=c.y || a.z!=c.z) {
			result.diffuse = float3(1, 1, 0);
			return result;
		}*/

		result.diffuse += diffuseColor;
		//result.specular = diffuseColor;
		//return result;
		//Specular
		float3 toLightDir = -dir;
		float3 half_v = normalize(toEyeDir + toLightDir);
		float specularCof = max(dot(half_v, (surfaceNormal)), 0.0f);
		float3 specularAtt = each.specularAttenuation;
		att = 1.0f / (1.0f + specularAtt.x + specularAtt.y * dist + specularAtt.z * dist * dist);
		specularCof = specularFactor * pow(specularCof, SPECULAR_POWER);
		float3 specularColor = each.color * each.intensity * att * specularCof;
		result.specular += specularColor;
	}
	result.diffuse = saturate(result.diffuse);
	return result;
}

VertToPixel Vert(VertInput input)
{
	VertToPixel result = (VertToPixel)0;

	float4 localPos = float4(input.position, 1.0f);
	float4 worldPos = mul(MODEL, localPos);
	float4 viewPos = mul(VIEW, worldPos);
	float4 clipPos = mul(PROJECTION, viewPos);

	float4 localNormal = float4(input.normal, 0.0f);
	float4 worldNormal = normalize(mul(MODEL, localNormal));
	float4 localTangent = float4(input.tangent, 0.0f);
	float4 worldTangent = normalize(mul(MODEL, localTangent));



	result.position = clipPos;
	result.color = input.color;
	result.normal = worldNormal.xyz;//; / worldNormal.w;
	result.texUV = input.texUV;
	result.worldPos = worldPos;
	result.tangent = worldTangent.xyz;
	return result;
}

float4 Pixel(VertToPixel input) : SV_TARGET0
{
	float4 surface = tAlbedo.Sample(sAlbedo, input.texUV);
	float3 normalV = tNormal.Sample(sNormal, input.texUV).xyz * float3(2, 2, 1) - float3(1, 1, 0);
	float4 specularColor = tSpecular.Sample(sSpecular, input.texUV);
	float specular = specularColor.x;
	float4 emm = tEmmisive.Sample(sEmmisive, input.texUV);

	float3 vTangent = normalize(input.tangent);
	float3 vNormal = normalize(input.normal);
	float3 vBitangent = normalize(cross(vNormal, vTangent));

	float3x3 tbnToWorld = transpose(float3x3(vTangent, vBitangent, vNormal));
	float3 worldNormal = mul(tbnToWorld, normalV);

	surface = pow(surface, GAMMA);
	float4 worldPos = input.worldPos;
	Lighting lightingColor = GetLighting(CAMERA_POSITION, worldPos.xyz / worldPos.w, worldNormal, specular);
	float4 color = float4(lightingColor.diffuse, 1.0f) * surface + float4(lightingColor.specular * specularColor.a, 0.0f) + emm * EMMISIVE;
	color = pow(color, 1.0f / GAMMA);

	//color = input.color;
	//color = float4(AMBIENT.xyz, 1.0f);
	/*float3 a = lightingColor.specular;
	float3 b = float3(1,1,1);
	float3 c = float3(1,1,1);
	a = a + b;
	if (a.x == c.x && a.y==c.y && a.z==c.z) {
		color = float4(1, 0, 0, 1);
	}*/

	float4 normalColor;
	normalColor = float4(normalize(worldNormal), 1.0f);
	normalColor.r = lerp(0, 1, (normalColor.r + 1.0f) / 2.f);
	normalColor.g = lerp(0, 1, (normalColor.g + 1.0f) / 2.f);
	normalColor.b = lerp(0, 1, (normalColor.b + 1.0f) / 2.f);

	float4 tangentColor;
	tangentColor = float4(normalize(vTangent), 1.0f);
	tangentColor.r = lerp(0, 1, (tangentColor.r + 1.0f) / 2.f);
	tangentColor.g = lerp(0, 1, (tangentColor.g + 1.0f) / 2.f);
	tangentColor.b = lerp(0, 1, (tangentColor.b + 1.0f) / 2.f);

	return lerp(lerp(lerp(color, normalColor, DRAW_NORMAL), tangentColor, DRAW_TANGENT), float4(lightingColor.specular, specular), DRAW_SPECULAR);
}
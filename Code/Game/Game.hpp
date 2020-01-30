#pragma once
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/WindowContext.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/RNG.hpp"
#include "Engine/Renderer/CPUMesh.hpp"
#include "Engine/Core/Vertex_PCUNT.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Material.hpp"

extern RenderContext* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern WindowContext* g_theWindow;

class Camera;
class Shader;

class Game
{
public:
	Game();
	~Game();
	
	bool IsRunning() const { return m_flagRunning; }
	void Startup();
	void BeginFrame();
	void Update(float deltaSeconds);
	void Render() const;

	void UpdateUI();

	void EndFrame();
	void Shutdown();

	void GetScreenSize(float *outputWidth, float *outputHeight) const;
	void SetScreenSize(float width, float height);

	const RenderContext* getRenderer() const { return g_theRenderer; }
	const InputSystem* getInputSystem() const { return g_theInput; }
	const AudioSystem* getAudioSystem() const { return g_theAudio; }
	void ToScreenShot(const std::string& path) { m_shotPath = path; m_screenshot = true; }
	RNG* getRNG() { return m_rng; }

	//IO
	void DoKeyDown(unsigned char keyCode);
	void DoKeyRelease(unsigned char keyCode);
	bool IsConsoleUp();
	void DoChar(char charCode);

//DEBUG
	void ToggleDebugView();

private:
	void _RenderDebugInfo(bool afterRender) const;

private:
	std::string m_shotPath;
	bool m_screenshot = false;
	bool m_flagRunning = false;
	float m_screenWidth;
	float m_screenHeight;
	Vec3 m_cameraPosition;
	float m_cameraSpeed = 0.0835f;
	Vec2 m_cameraRotationSpeedDegrees = Vec2(1.f, 1.f);
	Vec2 m_cameraRotation = Vec2::ZERO;
	RNG* m_rng = nullptr;
	Camera* m_mainCamera = nullptr;
	Shader* m_shader = nullptr;

	char ui_strbuf[256] = {};
	float ui_floatbuf = 0;

//DEBUG
	bool m_flagDebug = false;
	float m_upSeconds = 0.f;


	CPUMesh __cpuMeshForCube = CPUMesh(RenderBufferLayout::AcquireLayoutFor<Vertex_PCUNT>());
	GPUMesh* __gpuMeshForCube;
	TextureView2D* __cubeTexture;
	TextureView2D* __cubeNormal;
	TextureView2D* __cubeSpecular;



	CPUMesh __cpuMeshForUVS = CPUMesh(RenderBufferLayout::AcquireLayoutFor<Vertex_PCUNT>());
	GPUMesh* __gpuMeshForUVS;
	TextureView2D* __UVSTexture;
	TextureView2D* __UVSNormal;

	Material* _cubeMat;
};
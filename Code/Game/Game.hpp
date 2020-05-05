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
#include "Game/RVSGame.hpp"
extern RenderContext* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern WindowContext* g_theWindow;

class Camera;
class Shader;

class Game
{
public :
	static constexpr size_t MAX_ZONES = 2048 * 10;
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
	Vec2 get_mouse_in_world() const;
	
	//IO
	void DoKeyDown(unsigned char keyCode);
	void DoKeyRelease(unsigned char keyCode);
	bool IsConsoleUp();
	void DoChar(char charCode);

	void mouse_down();
	void mouse_up();
	void mouse_wheel(int delta);
	size_t m_num_zone = 10;
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


	RVSGame* m_rvsGame = nullptr;
	bool m_report_mouse = false;
//DEBUG
	bool m_flagDebug = false;
	float m_upSeconds = 0.f;
	AABB2 m_scene_ortho = AABB2(-1,-1,1,1);

	//TMP

	//std::vector<ConvexPoly> m_polys;
};
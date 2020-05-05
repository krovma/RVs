#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places

#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Develop/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Develop/DebugRenderer.hpp"
#include "Engine/Develop/UnitTest.hpp"
#include "Engine/Develop/Log.hpp"
#include "Engine/UI/UISystem.hpp"
#include "Engine/Develop/Profile.hpp"
#include "Engine/Core/Job.hpp"
#include <filesystem>

#include "Engine/Script/Py3.hpp"
#include "Engine/Core/StringUtils.hpp"

//////////////////////////////////////////////////////////////////////////
App* g_theApp = nullptr;
InputSystem* g_theInput = nullptr;
RenderContext* g_theRenderer = nullptr;
AudioSystem* g_theAudio = nullptr;
DevConsole* g_theConsole = nullptr;
WindowContext* g_theWindow;
//extern HWND g_hWnd;

//////////////////////////////////////////////////////////////////////////
bool AppQuit_Callback(EventParam& param)
{
	UNUSED(param);
	g_theApp->HandleQuitRequested();
	return true;
}
//////////////////////////////////////////////////////////////////////////
App::App()
{
}
//////////////////////////////////////////////////////////////////////////
App::~App()
{
	Shutdown();
}

//////////////////////////////////////////////////////////////////////////
void App::Startup()
{
	if (!std::filesystem::exists("logs")) {
		std::filesystem::create_directory("logs");
	}
	if (std::filesystem::exists("logs/default.log")) {
		std::string newname = "logs/default";
		for (int i = 1; i > 0; ++i) {
			std::string sname = Stringf("logs/default%d.log", i);
			if (!std::filesystem::exists(sname)) {
				newname = sname;
				break;
			}
		}
		std::filesystem::rename("logs/default.log", newname);
	}
	LogStart("logs/default.log");

	g_theJobSystem = new JobSystem();
	g_theJobSystem->Startup();

	g_theInput = new InputSystem();
	const IntVec2 windowRes(g_theWindow->GetClientResolution());
	g_theRenderer = new RenderContext(g_theWindow->m_hWnd, windowRes.x, windowRes.y);
	g_theAudio = new AudioSystem();
	g_theInput->StartUp();
	g_theRenderer->Startup();
	g_theConsole = new DevConsole(g_theRenderer, 72, 144);
	g_theConsole->Startup();
	DebugRenderer::Startup(g_theRenderer);
	g_theUI = new UISystem();
	g_theUI->Startup(g_theWindow, g_theRenderer);

	::RunUnitTest(ALL_UNIT_TEST, UNIT_TEST_LEVEL);

	m_theGame = new Game();
	m_theGame->Startup();

	g_Event->SubscribeEventCallback("quit", AppQuit_Callback);
}
//////////////////////////////////////////////////////////////////////////
void App::Shutdown()
{
	m_flagQuit = true;
	DebugRenderer::Shutdown();
	if (m_theGame) {
		m_theGame->Shutdown();
		delete m_theGame;
		m_theGame = nullptr;
	}

	if (g_theUI) {
		g_theUI->Shutdown();
		delete g_theUI;
		g_theUI = nullptr;
	}

	if (g_theConsole) {
		delete g_theConsole;
		g_theConsole = nullptr;
	}
	if (g_theRenderer) {
		g_theRenderer->Shutdown();
		delete g_theRenderer;
		g_theRenderer = nullptr;
	}
	if (g_theInput) {
		g_theInput->Shutdown();
		delete g_theInput;
		g_theInput = nullptr;
	}
	if (g_theAudio) {
		delete g_theAudio;
		g_theAudio = nullptr;
	}

	g_theJobSystem->Shutdown();

	while (!g_theJobSystem->IsFinished())
	{
		std::this_thread::yield();
	}

	LogStop();

}
//////////////////////////////////////////////////////////////////////////
void App::RunFrame()
{
	PROFILE_SCOPE(__FUNCTION__);

	static double lastFrameTime = GetCurrentTimeSeconds();
	g_theWindow->BeginFrame();
	g_theInput->BeginFrame();
	g_theAudio->BeginFrame();

	if (!m_theGame->IsRunning()) {
		m_flagQuit = true;
		return;
	}

	g_theUI->BeginFrame();

	m_theGame->BeginFrame();

	static double currentTime;
	currentTime = GetCurrentTimeSeconds();
	double dt = currentTime - lastFrameTime;
	if (m_flagPaused) {
		dt = 0.0;
	} else if (m_flagSlow) {
		dt /= 10.0;
	}
	DebugRenderer::Update(float(dt));

	m_theGame->Update(float(dt));
	m_theGame->Render();

	m_theGame->EndFrame();
	
	g_theUI->EndFrame();

	g_theAudio->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	
	for (int i = 0; i < NUM_JOB_TYPES;++i) {
		g_theJobSystem->FinishJobsQueue((JobType)i);
	}

	++m_frameCount;
	lastFrameTime = currentTime;
}
//////////////////////////////////////////////////////////////////////////
bool App::HandleKeyPressed(unsigned char keyCode)
{
	static int seed = 0;
	if (m_theGame->IsConsoleUp()) {
		m_theGame->DoKeyDown(keyCode);
	} else {
		if ('T' == keyCode) {
			m_flagSlow = true;
		} else if ('P' == keyCode) {
			m_flagPaused = !m_flagPaused;
		} else if (0x77 /*F8*/ == keyCode) {
			delete m_theGame;
			m_theGame = new Game();
			g_rng.Init(seed++,0);
			m_theGame->Startup();
		} else if (0x71 == keyCode /*F2*/) {

			if (!std::filesystem::exists("shots")) {
				std::filesystem::create_directory("shots");
			}
			if (std::filesystem::exists("shots/screenshoot.png")) {
				std::string newname = "logs/default";
				for (int i = 1; i > 0; ++i) {
					std::string sname = Stringf("shots/screenshoot.%d.png", i);
					if (!std::filesystem::exists(sname)) {
						newname = sname;
						break;
					}
				}
				std::filesystem::rename("shots/screenshoot.png", newname);
			}

			m_theGame->ToScreenShot("shots/screenshoot.png");
		
		} else {
			m_theGame->DoKeyDown(keyCode);
		}
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////
bool App::HandleKeyReleased(unsigned char keyCode)
{
	if (keyCode == 'T') {
		m_flagSlow = false;
	} else {
		m_theGame->DoKeyRelease(keyCode);
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////
bool App::HandleQuitRequested()
{
	m_flagQuit = true;
	return true;
}

bool App::HandleMouseButtonDown()
{
	m_theGame->mouse_down();
	return true;
}

bool App::HandleMouseButtonUp()
{
	m_theGame->mouse_up();
	return true;
}

bool App::HandleMouseWheel(int delta)
{
	m_theGame->mouse_wheel(delta);
	return true;
}

////////////////////////////////
bool App::HandleChar(char charCode)
{
	if (m_theGame->IsConsoleUp()) {
		m_theGame->DoChar(charCode);
	}
	return true;
}

UNIT_TEST(shouldNeverFailTest, "general", 10)
{
	return (0 == 0);
}

UNIT_TEST(anotherCoolTest, "general", 5)
{
	return (1 + 1 == 2);
}

UNIT_TEST(binary_file_test, "general", 10)
{
	//LoadFileToBuffer();
	return true;
}
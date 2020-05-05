#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
#include <cassert>
#include <crtdbg.h>
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Game/App.hpp"
#include "Engine/Event/EventSystem.hpp"
#include "Engine/UI/UISystem.hpp"
#include "ThirdParty/imgui/imgui.h"

constexpr float CLIENT_ASPECT = 1.f; // We are requesting a 1:1 aspect (square) window area


									  //-----------------------------------------------------------------------------------------------
									  // #SD1ToDo: Move each of these items to its proper place, once that place is established
									  // 
//HWND g_hWnd = nullptr;							// ...becomes WindowContext::m_windowHandle
//HDC g_displayDeviceContext = nullptr;			// ...becomes WindowContext::m_displayContext
//HGLRC g_openGLRenderingContext = nullptr;		// ...becomes RenderContext::m_apiRenderingContext
const char* APP_NAME = "RVs - Raycast vs Spaces";	// #ProgramTitle

NamedStrings g_gameConfigs;

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT(*_imguiProc) (HWND, UINT, WPARAM, LPARAM) = ImGui_ImplWin32_WndProcHandler;

bool WindowsMessageHandlingProcedure(void *hWnd, unsigned int wmMessageCode, unsigned int wParam, unsigned int lParam)
{
	UNUSED(lParam);
	UNUSED(hWnd);
#if defined(UI_USING_IMGUI)
	const ImGuiIO* io;
	if (g_theUI) {
		_imguiProc((HWND)hWnd, wmMessageCode, wParam, lParam);
		io = &ImGui::GetIO();
	}
#endif
	//DebuggerPrintf("\t\t\t\t\t\t\t\t\t\t\t\t\t%x\n", wmMessageCode);
	switch (wmMessageCode) {
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
	case WM_CLOSE: {
		g_theApp->HandleQuitRequested();
		return true; // "Consumes" this message (tells Windows "okay, we handled it")
		}

	// Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
	case WM_KEYDOWN: {
		unsigned char asKey = (unsigned char)wParam;

		g_theApp->HandleKeyPressed(asKey);
		return true;
		}

	// Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
	case WM_KEYUP: {
		unsigned char asKey = (unsigned char) wParam;

		g_theApp->HandleKeyReleased(asKey);
		return true;
		}
	case WM_CHAR: {
#if defined(UI_USING_IMGUI)
		if (io->WantCaptureKeyboard) {
			return false;
		}
#endif
		char asChar = (char)wParam;
		g_theApp->HandleChar(asChar);
		return true;
		}
	case WM_LBUTTONDOWN: {
		g_theApp->HandleMouseButtonDown();
		return true;
	}
	case WM_LBUTTONUP: {
		g_theApp->HandleMouseButtonUp();
		return true;
	}
	case WM_MOUSEWHEEL: {
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		g_theApp->HandleMouseWheel(zDelta);
		return true;
	}
			
	}

	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return false;
}

#include "Engine/Develop/Memory.hpp"
#include "Engine/Develop/Profile.hpp"
//-------------------------------------------------------------------------------
void Startup()
{
// 	void* p = TrackedAlloc(sizeof(int));
// 	TrackedFree(p);
	ProfileInit();
	XmlElement* gameConfigXmlRoot = nullptr;
	ParseXmlFromFile(gameConfigXmlRoot, "Data/GameConfig.xml");
	if(gameConfigXmlRoot == nullptr) {
		ERROR_AND_DIE(Stringf("Cannot Find GameConfig.xml! Is the running path set to %s/Run?", APP_NAME));
	}
	g_gameConfigs.PopulateFromXmlElement(*gameConfigXmlRoot);

	g_theWindow = new WindowContext();
	g_theWindow->Create(APP_NAME, CLIENT_ASPECT, 0.9f, WindowsMessageHandlingProcedure);
	g_theApp = new App();
	g_theApp->Startup();
	//g_Event = new EventSystem();
	//g_Event->Startup();

}


//-----------------------------------------------------------------------------------------------
void Shutdown()
{
	// Destroy the global App instance
	delete g_theApp;
	g_theApp = nullptr;

	delete g_Event;
	g_Event = nullptr;

	delete g_theWindow;
	g_theWindow = nullptr;
}


//-----------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int)
{
	UNUSED(commandLineString);
	UNUSED(applicationInstanceHandle);
	Startup();

	// Program main loop; keep running frames until it's time to quit
	while (!g_theApp->IsQuitting())
	{
		//RunMessagePump();
		g_theApp->RunFrame();
		//SwapBuffers(g_displayDeviceContext);
		//Sleep(16);// Fake 60fps
	}

	Shutdown();
	return 0;
}

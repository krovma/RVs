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

constexpr float CLIENT_ASPECT = 1.5f; // We are requesting a 1:1 aspect (square) window area


									  //-----------------------------------------------------------------------------------------------
									  // #SD1ToDo: Move each of these items to its proper place, once that place is established
									  // 
//HWND g_hWnd = nullptr;							// ...becomes WindowContext::m_windowHandle
//HDC g_displayDeviceContext = nullptr;			// ...becomes WindowContext::m_displayContext
//HGLRC g_openGLRenderingContext = nullptr;		// ...becomes RenderContext::m_apiRenderingContext
const char* APP_NAME = "Protogame3D";	// #ProgramTitle

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
	}

	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return false;
}

/*
void CreateAppWindow(HINSTANCE appInstanceHandle, float clientAspect)
{
	WNDCLASSEX windowClassDescription;
	memset(&windowClassDescription, 0, sizeof(windowClassDescription));
	windowClassDescription.cbSize = sizeof(windowClassDescription);
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
	windowClassDescription.hInstance = GetModuleHandle(NULL);
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT("Protogame2D"); // #ProgramTitle
	RegisterClassEx(&windowClassDescription);

	// #SD1ToDo: Add support for fullscreen mode (requires different window style flags than windowed mode)
	const DWORD windowStyleFlags = WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED;
	const DWORD windowStyleExFlags = WS_EX_APPWINDOW;

	// Get desktop rect, dimensions, aspect
	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &desktopRect);
	float desktopWidth = (float)(desktopRect.right - desktopRect.left);
	float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);
	float desktopAspect = desktopWidth / desktopHeight;

	// Calculate maximum client size (as some % of desktop size)
	constexpr float maxClientFractionOfDesktop = 0.90f;
	float clientWidth = desktopWidth * maxClientFractionOfDesktop;
	float clientHeight = desktopHeight * maxClientFractionOfDesktop;
	if (clientAspect > desktopAspect) {
		// Client window has a wider aspect than desktop; shrink client height to match its width
		clientHeight = clientWidth / clientAspect;
	} else {
		// Client window has a taller aspect than desktop; shrink client width to match its height
		clientWidth = clientHeight * clientAspect;
	}

	// Calculate client rect bounds by centering the client area
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);
	RECT clientRect;
	clientRect.left = (int)clientMarginX;
	clientRect.right = clientRect.left + (int)clientWidth;
	clientRect.top = (int)clientMarginY;
	clientRect.bottom = clientRect.top + (int)clientHeight;

	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;
	AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

	WCHAR windowTitle[1024];
	MultiByteToWideChar(GetACP(), 0, APP_NAME, -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));
	g_hWnd = CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		appInstanceHandle,
		NULL);

	GUARANTEE_OR_DIE(nullptr!=g_hWnd, "Failed to create window\n");

	ShowWindow(g_hWnd, SW_SHOW);
	SetForegroundWindow(g_hWnd);
	SetFocus(g_hWnd);

	g_displayDeviceContext = GetDC(g_hWnd);

	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	SetCursor(cursor);
}



//-----------------------------------------------------------------------------------------------
// Processes all Windows messages (WM_xxx) for this app that have queued up since last frame.
// For each message in the queue, our WindowsMessageHandlingProcedure (or "WinProc") function
//	is called, telling us what happened (key up/down, minimized/restored, gained/lost focus, etc.)
//
// #SD1ToDo: We will move this function to a more appropriate place later on...
//
void RunMessagePump()
{
	MSG queuedMessage;
	while(true) {
		const BOOL wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent) {
			break;
		}
		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
	}
}
*/
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
	g_theWindow->Create(APP_NAME, CLIENT_ASPECT, 0.8f, WindowsMessageHandlingProcedure);
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

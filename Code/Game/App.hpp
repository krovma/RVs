#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Game/Game.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/WindowContext.hpp"
#include "Engine/Audio/AudioSystem.hpp"

//////////////////////////////////////////////////////////////////////////
class App
{
public:
	App();
	~App();

	void Startup();
	void Shutdown();
	void RunFrame();

	bool IsQuitting() { return m_flagQuit; }
	bool HandleKeyPressed(unsigned char keyCode);
	bool HandleKeyReleased(unsigned char keyCode);
	bool HandleQuitRequested();
	bool HandleMouseButtonDown();
	bool HandleMouseButtonUp();
	bool HandleMouseWheel(int delta);
	bool HandleChar(char charCode);

private:

	Game* m_theGame = nullptr;
	
	int m_frameCount = 0;
	bool m_flagQuit = false;
	bool m_flagPaused = false;
	bool m_flagSlow = false;

};

extern App* g_theApp;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern WindowContext* g_theWindow;

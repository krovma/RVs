#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUNT.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/TextureView2D.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteAnimationDef.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Develop/DevConsole.hpp"
#include "Engine/Event/EventSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include <cmath>
#include "Engine/Renderer/RenderBuffer.hpp"
#include "Engine/Develop/DebugRenderer.hpp"
#include "Engine/Renderer/CPUMesh.hpp"
#include "Engine/Renderer/GPUMesh.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Entity.hpp"
#include "Engine/Develop/Log.hpp"
#include "Engine/UI/UISystem.hpp"
#include "Engine/Develop/Profile.hpp"
#include "ThirdParty/imgui/imgui.h"
//////////////////////////////////////////////////////////////////////////
//Delete these globals
//////////////////////////////////////////////////////////////////////////

ConstantBuffer *g_frameBuffer = nullptr;
//////////////////////////////////////////////////////////////////////////
struct FrameBuffer_t
{
	float time=0.f; float gamma = 1.f;
	float useNormal = 0.f; float useTangent = 0.f; float useSpecular = 0.f;
	float emmisive = 0.f;
	float __[2];
};
FrameBuffer_t _frameBufferContent;
//////////////////////////////////////////////////////////////////////////
Game::Game()
{
	m_rng = new RNG();
	m_rng->Init();
	m_flagRunning = true;
}

Game::~Game()
{
	Shutdown();
}

static Entity* sth[10];
static int top = 0;
static bool _alloc_cmd(NamedStrings& param)
{
	if (top < 10) {
		sth[top] = new Entity();
		++top;
	}
	return true;
}
static bool _free_cmd(NamedStrings& param)
{
	if (top > 0) {
		--top;
		delete sth[top];
	}
	return true;
}
#include "Engine/Develop/Memory.hpp"
static bool _logmem_cmd(NamedStrings& param)
{
	LogLiveAllocations();
	return true;
}

static bool _Log_cmd(NamedStrings& param)
{
	std::string filter = param.GetString("filter", "default");
	std::string msg = param.GetString("msg", "nothing");
	Log(filter.c_str(), msg.c_str());
	return true;
}

static bool _Log_Filter_EnableAll(NamedStrings& param)
{
	LogFilterEnableAll();
	return true;
}
static bool _Log_Filter_DisableAll(NamedStrings& param)
{
	LogFilterDisableAll();
	return true;
}

static bool _Log_Filter_Enable(NamedStrings& param)
{
	std::string filter = param.GetString("filter", "default");
	bool enable = param.GetBool("off", true);
	if (enable) {
		LogFilterEnable(filter);
	} else {
		LogFilterDisable(filter);
	}
	return true;
}

static bool _Log_Flush(NamedStrings& param)
{
	Log("debug", "This message was logged.");
	LogFlush();

	// put breakpoint here, open log file and make sure above line was written; 
	int i = 0;
	i = i;
	return true;
}

static bool _Profile_Report(NamedStrings& param)
{
	int frameReveredN = param.GetInt("f", 0);
	std::string total = param.GetString("total", "No");
	ProfilerNode* tree = RequireReferenceOfProfileTree(std::this_thread::get_id(), frameReveredN);
	ShowTreeView(tree, total=="No");
	ProfileReleaseTree(tree);
	return true;
}

static bool _Profile_Report_Flat(NamedStrings& param)
{
	int frameReveredN = param.GetInt("f", 0);
	std::string total = param.GetString("total", "No");
	ProfilerNode* tree = RequireReferenceOfProfileTree(std::this_thread::get_id(), frameReveredN);
	ShowFlatView(tree, total=="No");
	ProfileReleaseTree(tree);
	return true;
}

void Game::Startup()
{
	g_theWindow->LockMouse();
	g_theWindow->SetMouseInputMode(MOUSE_INPUT_ABSOLUTE);
	g_theWindow->HideMouse();
	
	m_cameraPosition = Vec3(0, 0, 0);
	m_mainCamera = new Camera();
	m_shader = Shader::CreateShaderFromXml("Data/Shaders/unlit.shader.xml", g_theRenderer);
	//m_shader->SetDepthStencil(COMPARE_GREATEREQ, true);
	//m_shader = Shader::CreateShaderFromXml("Data/Shaders/lit.shader.xml", g_theRenderer);
	//float aspect = m_screenWidth / m_screenHeight;
	//m_mainCamera->SetOrthoView(Vec2(-5.f,-5.f), Vec2(5.f, 5.f), -0.001f, -100.0f);

	DevConsole::s_consoleFont =
		g_theRenderer->AcquireBitmapFontFromFile(
			g_gameConfigs.GetString("consoleFont", "SquirrelFixedFont").c_str()
		);


	g_frameBuffer = new ConstantBuffer(g_theRenderer);

	g_Event->SubscribeEventCallback("test_alloc", _alloc_cmd);
	g_Event->SubscribeEventCallback("test_free", _free_cmd);
	g_Event->SubscribeEventCallback("logmem", _logmem_cmd);

	g_Event->SubscribeEventCallback("filterall", _Log_Filter_DisableAll);
	g_Event->SubscribeEventCallback("filternone", _Log_Filter_EnableAll);
	g_Event->SubscribeEventCallback("filter", _Log_Filter_Enable);
	g_Event->SubscribeEventCallback("logflush", _Log_Flush);
	g_Event->SubscribeEventCallback("log", _Log_cmd);

	g_Event->SubscribeEventCallback("report", _Profile_Report);
	g_Event->SubscribeEventCallback("flat_report", _Profile_Report_Flat);




	for (size_t i = 0; i< 10;++i) {
		m_polys.emplace_back(ConvexPoly::GetRandomPoly(g_rng.GetFloatInRange(0.1f, 0.3f)));
	}


	
	Log("Game", "Game start");
}

void Game::BeginFrame()
{
	g_theRenderer->BeginFrame();
	g_theConsole->BeginFrame();
}
#include "Engine/Develop/Profile.hpp"
void Game::Update(float deltaSeconds)
{
	//PROFILE_SCOPE(__FUNCTION__);
	m_upSeconds += deltaSeconds;
	_frameBufferContent.time = m_upSeconds;
	g_theConsole->Update(deltaSeconds);

	
	//Vec2 mouseMove(g_theWindow->GetClientMouseRelativeMovement());
	//mouseMove *= -0.1f;
	//m_cameraRotation += Vec2(mouseMove.y * m_cameraRotationSpeedDegrees.y, mouseMove.x * m_cameraRotationSpeedDegrees.x);

	IntVec2 mousePosition = g_theWindow->GetClientMousePosition();
	static IntVec2 CLIENT_SIZE = g_theWindow->GetClientResolution();
	DebugRenderer::Log(Stringf("%i %i", mousePosition.x, mousePosition.y, CLIENT_SIZE.x, CLIENT_SIZE.y), 0, Rgba::BLACK);
	Vec2 mouse_in_world = Vec2(FloatMap(mousePosition.x, 0, CLIENT_SIZE.x, -1, 1)
	, FloatMap(mousePosition.y, 0, CLIENT_SIZE.y, 1, -1));
	DebugRenderer::DrawPoint3D(Vec3(mouse_in_world, 0.5f), 0.01f, 0, Rgba::GREEN);

	UpdateUI();
	
}

void Game::Render() const
{
	PROFILE_SCOPE(__FUNCTION__);

	RenderTargetView* renderTarget = g_theRenderer->GetFrameColorTarget();
	m_mainCamera->SetRenderTarget(renderTarget);
	DepthStencilTargetView* dstTarget = g_theRenderer->GetFrameDepthStencilTarget();
	m_mainCamera->SetDepthStencilTarget(dstTarget);
	m_mainCamera->SetProjection(Camera::MakeOrthogonalProjection(Vec2(-1, -1), Vec2(1, 1), 1, -1));
	Mat4 cameraModel =// = Mat4::Identity; //T*R*S
	 Mat4::MakeTranslate3D(Vec3(0, 0, 0));
	m_mainCamera->SetCameraModel(cameraModel);
	//DebugRenderer::DrawCameraBasisOnScreen(*m_mainCamera, 0.f);
	g_theRenderer->BeginCamera(*m_mainCamera);
	g_theRenderer->ClearColorTarget(Rgba::WHITE);
	g_theRenderer->ClearDepthStencilTarget(0.f);
	g_theRenderer->BindShader(m_shader);
	m_shader->ResetShaderStates();


	std::vector<Vertex_PCU> verts;
	for (auto each:m_polys) {
		for (size_t i = 1; i < each.m_points.size(); ++ i) {
			AddVerticesOfLine2D(verts, each.m_points[i - 1], each.m_points[i], 0.002f, Rgba::BLACK);
		}
		AddVerticesOfLine2D(verts, each.m_points[each.m_points.size() - 1], each.m_points[0], 0.002f, Rgba::BLACK);
	}

	
	//_frameBufferContent.emmisive =  (sin(m_upSeconds) + 1.f)*0.5f;
	g_frameBuffer->Buffer(&_frameBufferContent, sizeof(_frameBufferContent));
	g_theRenderer->BindConstantBuffer(CONSTANT_SLOT_FRAME, g_frameBuffer);
	g_theRenderer->DrawVertexArray(verts.size(), verts);
	//ConstantBuffer* model = g_theRenderer->GetModelBuffer();
	//g_theRenderer->BindConstantBuffer(CONSTANT_SLOT_MODEL, model);

	_RenderDebugInfo(true);
	
	g_theRenderer->EndCamera(*m_mainCamera);
}

////////////////////////////////
void Game::UpdateUI()
{
}

void Game::_RenderDebugInfo(bool afterRender) const
{
	PROFILE_SCOPE(__FUNCTION__);
	if (afterRender) {
		//g_theRenderer->BindShader()
		DebugRenderer::Render(m_mainCamera);
		g_theConsole->RenderConsole();
		g_theUI->Render();
	}
}

void Game::EndFrame()
{
	if (m_screenshot) {
		m_screenshot = false;
		g_theRenderer->Screenshoot(m_shotPath);
	}
	g_theRenderer->EndFrame();
	g_theConsole->EndFrame();
}

void Game::Shutdown()
{
	g_theWindow->UnlockMouse();
	//delete _timeBuffer;
	delete m_mainCamera;
	delete g_frameBuffer;
}

void Game::GetScreenSize(float *outputWidth, float *outputHeight) const
{
	*outputHeight = m_screenHeight;
	*outputWidth = m_screenWidth;
}

void Game::SetScreenSize(float width, float height)
{
	m_screenWidth = width;
	m_screenHeight = height;
}

void Game::DoKeyDown(unsigned char keyCode)
{
	//g_theAudio->PlaySound(tmpTestSound);
	if (IsConsoleUp()) {
		if (keyCode == KEY_ESC) {
			g_theConsole->KeyPress(CONSOLE_ESC);
		}	else if (keyCode == KEY_ENTER) {
			g_theConsole->KeyPress(CONSOLE_ENTER);
		} else if (keyCode == KEY_BACKSPACE) {
			g_theConsole->KeyPress(CONSOLE_BACKSPACE);
		} else if (keyCode == KEY_LEFTARROW) {
			g_theConsole->KeyPress(CONSOLE_LEFT);
		} else if (keyCode == KEY_RIGHTARROW) {
			g_theConsole->KeyPress(CONSOLE_RIGHT);
		} else if (keyCode == KEY_UPARROW) {
			g_theConsole->KeyPress(CONSOLE_UP);
		} else if (keyCode == KEY_DOWNARROW) {
			g_theConsole->KeyPress(CONSOLE_DOWN);
		} else if (keyCode == KEY_DELETE) {
			g_theConsole->KeyPress(CONSOLE_DELETE);
		} else if (keyCode == KEY_F6) {
			g_Event->Trigger("test", g_gameConfigs);
			//g_theConsole->RunCommandString("Test name=F6 run=true");
		} else if (keyCode == KEY_F1) {
			g_Event->Trigger("random");
		}
		return;
	}
	Mat4 rotation = Mat4::MakeRotationXYZ(m_cameraRotation.x, m_cameraRotation.y, 0.f);
	if (keyCode == KEY_SLASH) {
		if (g_theConsole->GetConsoleMode() == CONSOLE_OFF) {
			//ProfilePause();
			g_theConsole->SetConsoleMode(CONSOLE_PASSIVE);
		} else {
			//ProfileResume();
			g_theConsole->SetConsoleMode(CONSOLE_OFF);
		}
	} else if (keyCode == KEY_W) {
		Vec4 move = rotation * Vec4(0, 0, -m_cameraSpeed, 0);
		m_cameraPosition += Vec3(move.x, move.y, move.z);
	} else if (keyCode == KEY_A) {
		Vec4 move = rotation * Vec4(-m_cameraSpeed, 0, 0, 0);
		m_cameraPosition += Vec3(move.x, move.y, move.z);
	} else if (keyCode == KEY_S) {
		Vec4 move = rotation * Vec4(0, 0, m_cameraSpeed, 0);
		m_cameraPosition += Vec3(move.x, move.y, move.z);
	} else if (keyCode == KEY_D) {
		Vec4 move = rotation * Vec4(m_cameraSpeed, 0, 0, 0);
		m_cameraPosition += Vec3(move.x, move.y, move.z);
	}

}

void Game::DoKeyRelease(unsigned char keyCode)
{
	UNUSED(keyCode);
}

////////////////////////////////
bool Game::IsConsoleUp()
{
	return (g_theConsole->GetConsoleMode() == CONSOLE_PASSIVE);
}

////////////////////////////////
void Game::DoChar(char charCode)
{
	if (!IsConsoleUp())
		return;
	g_theConsole->Input(charCode);
}

void Game::ToggleDebugView()
{
	m_flagDebug = !m_flagDebug;
}

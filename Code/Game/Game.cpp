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

static bool _toggleNormal(NamedStrings& param) {
	UNUSED(param);
	_frameBufferContent.useNormal = 1.f - _frameBufferContent.useNormal;
	return true;
}
static bool _toggleSpecular(NamedStrings& param)
{
	UNUSED(param);
	_frameBufferContent.useSpecular = 1.f - _frameBufferContent.useSpecular;
	return true;
}
static bool _toggleTangent(NamedStrings& param)
{
	UNUSED(param);
	_frameBufferContent.useTangent = 1.f - _frameBufferContent.useTangent;
	return true;
}

static bool _setAmbientLight(NamedStrings& param)
{
	Rgba amb = param.GetRgba("color", Rgba::TRANSPARENT_BLACK);
	g_theRenderer->SetAmbientLight(amb, amb.a);
	return true;
}

static bool _setSpecular(NamedStrings& param)
{
	float f = param.GetFloat("f", 0.f);
	float p = param.GetFloat("p", 1.f);
	g_theRenderer->SetSpecularFactors(f, p);
	return true;
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
	//m_mainCamera = new Camera(Vec2(0, 0), Vec2(200, 100));
	g_theWindow->LockMouse();
	//g_theWindow->SetMouseInputMode(MOUSE_INPUT_RELATIVE);
	//g_theWindow->HideMouse();
	
	m_cameraPosition = Vec3(0, 0, 0);
	m_mainCamera = new Camera();
	//m_shader = g_theRenderer->AcquireShaderFromFile("Data/Shaders/unlit.hlsl");
	//m_shader->SetDepthStencil(COMPARE_GREATEREQ, true);
	m_shader = Shader::CreateShaderFromXml("Data/Shaders/lit.shader.xml", g_theRenderer);
	//float aspect = m_screenWidth / m_screenHeight;
	//m_mainCamera->SetOrthoView(Vec2(-5.f,-5.f), Vec2(5.f, 5.f), -0.001f, -100.0f);

	DevConsole::s_consoleFont =
		g_theRenderer->AcquireBitmapFontFromFile(
			g_gameConfigs.GetString("consoleFont", "SquirrelFixedFont").c_str()
		);
/*
	float m[] = {
		1,2,3,4,
		1,1,2,3,
		1,1,1,2,
		1,1,1,1,
	};
	Mat4 mat(m);
	Mat4 inv(mat.GetInverted());
	m[0];*/

	g_frameBuffer = new ConstantBuffer(g_theRenderer);

	AABB3 cube(Vec3(-1,-1,-1), Vec3(1, 1, 1));

	__cpuMeshForCube.Clear();
	__cpuMeshForCube.AddCubeToMesh(cube);
	//__cpuMeshForCube.SetLayout(RenderBufferLayout::AcquireLayoutFor<Vertex_PCUN>());
	__gpuMeshForCube = new GPUMesh(g_theRenderer);
	__gpuMeshForCube->CreateFromCPUMesh(__cpuMeshForCube);
	__cubeTexture = g_theRenderer->AcquireTextureViewFromFile("Data/Images/example_color.png");
	__cubeNormal = g_theRenderer->AcquireTextureViewFromFile("Data/Images/example_normal.png");
	__cubeSpecular = g_theRenderer->AcquireTextureViewFromFile("Data/Images/example_spec.png");


	//__cubeTexture = nullptr;
	__cpuMeshForUVS.Clear();
	//__cpuMeshForUVS.AddUVSphereToMesh(Vec3(0,0,0), 1.f);
	//__cpuMeshForUVS.AddCylinderToMesh(Vec3(0, 0, 0), Vec3(1, 0, 0), 0.1f);
	__cpuMeshForUVS.AddUVSphereToMesh(Vec3(0, 0, 0), 2.f);
	//__cpuMeshForUVS.SetLayout(RenderBufferLayout::AcquireLayoutFor<Vertex_PCUN>());
	__gpuMeshForUVS = new GPUMesh(g_theRenderer);
	__gpuMeshForUVS->CreateFromCPUMesh(__cpuMeshForUVS);
	__UVSTexture = g_theRenderer->AcquireTextureViewFromFile("Data/Images/ruinwall_color.png");
	__UVSNormal = g_theRenderer->AcquireTextureViewFromFile("Data/Images/ruinwall_normal.png");


	RenderContext::Light dirLight;
	dirLight.color = Vec3(1, 1, 1);
	dirLight.isDirectional = 1.f;
	dirLight.direction = Vec3(-1, -1, 0);
	dirLight.intensity = 4.f;

	g_theRenderer->SetAmbientLight(Rgba::WHITE, 0.0f);
	g_theRenderer->EnableLight(0,dirLight);
	
	/*dirLight.color = Vec3(1, 0, 0);
	dirLight.direction = Vec3(0, -1, 0);
	g_theRenderer->EnableLight(1, dirLight);*/

	_cubeMat = Material::AcquireMaterialFromFile(g_theRenderer, "Data/Materials/coach.mat.xml");

	g_Event->SubscribeEventCallback("normal", _toggleNormal);
	g_Event->SubscribeEventCallback("specular", _toggleSpecular);
	g_Event->SubscribeEventCallback("tangent", _toggleTangent);

	g_Event->SubscribeEventCallback("ambient", _setAmbientLight);
	g_Event->SubscribeEventCallback("specular", _setSpecular);

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
	PROFILE_SCOPE(__FUNCTION__);
	m_upSeconds += deltaSeconds;
	_frameBufferContent.time = m_upSeconds;
	g_theConsole->Update(deltaSeconds);

	
	Vec2 mouseMove(g_theWindow->GetClientMouseRelativeMovement());
	mouseMove *= -0.1f;
	m_cameraRotation += Vec2(mouseMove.y * m_cameraRotationSpeedDegrees.y, mouseMove.x * m_cameraRotationSpeedDegrees.x);
	
	RenderContext::Light pointLight;
	Vec3 pos(0.f, 2.f * CosDegrees(30.f * m_upSeconds), 2.f * SinDegrees(30.f * m_upSeconds) - 1.f);
	pointLight.color = Vec3(1, 1, 1);
	pointLight.isDirectional = 0.f;
	pointLight.intensity = 1.f;
	pointLight.position = pos;
	g_theRenderer->EnableLight(2, pointLight);
	DebugRenderer::DrawPoint3D(pos, 0.05f, 0.f, Rgba(0.f, 1, 0));

	/*pos = Vec3(2.f * CosDegrees(30.f * m_upSeconds) - 1.f ,0.f, 2.f * SinDegrees(30.f * m_upSeconds) - 1.f);
	pointLight.color = Vec3(1, 1, 0);
	pointLight.isDirectional = 0.f;
	pointLight.intensity = 1.f;
	pointLight.position = pos;
	g_theRenderer->EnableLight(3, pointLight);
	DebugRenderer::DrawPoint3D(pos, 0.05f, 0.f, Rgba(1.f, 1, 0));*/

	__cubeTexture = g_theRenderer->AcquireTextureViewFromFile("Data/Images/example_color.png");
	__cubeNormal = g_theRenderer->AcquireTextureViewFromFile("Data/Images/example_normal.png");
	__cubeSpecular = g_theRenderer->AcquireTextureViewFromFile("Data/Images/example_spec.png");
	__UVSTexture = g_theRenderer->AcquireTextureViewFromFile("Data/Images/ruinwall_color.png");
	__UVSNormal = g_theRenderer->AcquireTextureViewFromFile("Data/Images/ruinwall_normal.png");


	UpdateUI();
	
}

void Game::Render() const
{
	PROFILE_SCOPE(__FUNCTION__);

	RenderTargetView* renderTarget = g_theRenderer->GetFrameColorTarget();
	m_mainCamera->SetRenderTarget(renderTarget);
	DepthStencilTargetView* dstTarget = g_theRenderer->GetFrameDepthStencilTarget();
	m_mainCamera->SetDepthStencilTarget(dstTarget);
	m_mainCamera->SetProjection(Camera::MakePerspectiveProjection(
		60.f,
		renderTarget->GetAspect(),
		-0.1f,
		-100.f
	));
	Mat4 cameraModel; //T*R*S
	cameraModel = Mat4::MakeTranslate3D(m_cameraPosition) * Mat4::MakeRotationXYZ(m_cameraRotation.x, m_cameraRotation.y, 0.f);
	m_mainCamera->SetCameraModel(cameraModel);
	DebugRenderer::DrawCameraBasisOnScreen(*m_mainCamera, 0.f);
	g_theRenderer->BeginCamera(*m_mainCamera);
	g_theRenderer->ClearColorTarget(Rgba::BLACK);
	g_theRenderer->ClearDepthStencilTarget(0.f);
	g_theRenderer->BindShader(m_shader);
	m_shader->ResetShaderStates();

	_frameBufferContent.emmisive = (sin(m_upSeconds) + 1.f)*0.5f;
	g_frameBuffer->Buffer(&_frameBufferContent, sizeof(_frameBufferContent));
	g_theRenderer->BindConstantBuffer(CONSTANT_SLOT_FRAME, g_frameBuffer);
	ConstantBuffer* model = g_theRenderer->GetModelBuffer();
	g_theRenderer->BindConstantBuffer(CONSTANT_SLOT_MODEL, model);
	
	Mat4 modelMat = Mat4::MakeTranslate3D(Vec3(-2, 0, -5)) * Mat4::MakeRotationXYZ(0, m_upSeconds * 30.f, 0);
	model->Buffer(&modelMat, sizeof(modelMat));
	_cubeMat->UseMaterial(g_theRenderer);
	g_theRenderer->DrawMesh(*__gpuMeshForCube);

	modelMat = Mat4::MakeTranslate3D(Vec3(2, 0, -3)) * Mat4::MakeRotationXYZ(m_upSeconds * 15.f, m_upSeconds * 30.f, 0);// *Mat4::Identity;
	model->Buffer(&modelMat, sizeof(modelMat));
	g_theRenderer->BindTextureViewWithSampler(TEXTURE_SLOT_DIFFUSE, __UVSTexture);
	g_theRenderer->BindTextureViewWithSampler(TEXTURE_SLOT_NORMAL, __UVSNormal);
	g_theRenderer->BindTextureViewWithSampler(TEXTURE_SLOT_EMMISIVE, nullptr);
	g_theRenderer->DrawMesh(*__gpuMeshForUVS);

	/*
	// Check TBN Base
	for (auto eachPoint : __cpuMeshForUVS.GetRawData()) {
		Vec3 pos = (modelMat * Vec4(eachPoint.Position, 1.f)).XYZ();
		Vec3 normal = (modelMat * Vec4(eachPoint.Normal, 0.f)).XYZ();
		Vec3 tan = (modelMat * Vec4(eachPoint.Tangent, 0.f)).XYZ();
		Vec3 bitan = normal.CrossProduct(tan).GetNormalized();
		DebugRenderer::DrawLine3D(pos, pos + tan * 0.05f, 0.005f, 0.f, Rgba::RED);
		DebugRenderer::DrawLine3D(pos, pos + bitan * 0.05f, 0.005f, 0.f, Rgba::GREEN);
		DebugRenderer::DrawLine3D(pos, pos + normal * 0.05f, 0.005f, 0.f, Rgba::BLUE);
	}*/

	_RenderDebugInfo(true);
	
	g_theRenderer->EndCamera(*m_mainCamera);
}

////////////////////////////////
void Game::UpdateUI()
{
	//PROFILE_SCOPE(__FUNCTION__);
	//ImGui::Begin("ImGui Test", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	//auto pos = g_theWindow->GetClientMousePosition();
	//ImGui::Text("%d %d", pos.x, pos.y);
	//if (ImGui::Button("Save"))
	//{
		//g_theConsole->Print("[ImGui] Button Clicked");
	//}
	//ImGui::InputText("string", ui_strbuf, IM_ARRAYSIZE(ui_strbuf));
	//ImGui::SliderFloat("float", &ui_floatbuf, 0.0f, 1.0f);
	//ImGui::End();
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
	delete __gpuMeshForCube;
	delete __gpuMeshForUVS;
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

#include "precomp.h"
#include <chrono>

#include "Graphics.h"
#include "game.h"
#include "InputManager.h"

#ifdef PLATFORM_WINDOWS
#include "GraphicsWindows.h"
#include "imgui_impl_glfw.h"

#elif PLATFORM_LINUX
#include "GraphicsLinux.h"
#include <filesystem>
#endif

#include "Settings.h"
#include "Scope.h"
#include "Texture.h"

int main()
{
	Framework::InputManager& inputManager = Framework::InputManager::Inst();

#ifdef PLATFORM_WINDOWS
	LOGMESSAGE("Platform is windows");
	std::unique_ptr<Graphics> graphics = std::make_unique<GraphicsWindows>();

#elif PLATFORM_LINUX
	LOGMESSAGE("Platform is linux");
	std::unique_ptr<Graphics> graphics = std::make_unique<GraphicsLinux>();
#else
	static_assert(false && "Platform is undefined!");
#endif // PLATFORM_WINDOWS

	graphics->Init(sScreenWidth, sScreenHeight);
	inputManager.Init(graphics.get());

#ifdef PLATFORM_LINUX
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = { sScreenWidth, sScreenHeight };

	ImGui_ImplOpenGL3_Init("#version 300 es");
#elif PLATFORM_WINDOWS
	// Initialize imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO io = ImGui::GetIO(); (void)io;

	ImGui_ImplGlfw_InitForOpenGL(static_cast<GraphicsWindows*>(graphics.get())->mWindow, true);
	ImGui_ImplOpenGL3_Init("#version 130");
#endif // PLATFORM_LINUX
	CheckGL();

	float deltaTime;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t2{};

	glClearColor(.5f, .5f, 1.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);

	{
		Framework::Settings& settings = Framework::Settings::Inst();
		Framework::Data::Scope settingsScope = settings.GetSettings();

		int currentMaxSize;
		Framework::Data::Variable& textureSizeVar = settingsScope.GetVariable("maxTextureSize");
		textureSizeVar >> currentMaxSize;

		int deviceTextureSizeLimit = Framework::Texture::GetMaxTextureSizeForDevice();
		currentMaxSize = std::min(deviceTextureSizeLimit, currentMaxSize);
		textureSizeVar << currentMaxSize;

		settings.SetSettings(settingsScope);

		ImGui::GetIO().Fonts->TexDesiredWidth = deviceTextureSizeLimit;
	}

	Framework::Game* game = new Framework::Game;
	game->Init();

	bool gameRunning = true;

	while (gameRunning)
	{
		game->EarlyTick();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		inputManager.NewFrame();
		graphics->NewFrame();

		t2 = std::chrono::high_resolution_clock::now();
		deltaTime = (std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1)).count();
		t1 = t2;
				
		gameRunning = game->Tick(deltaTime);

		graphics->Render();
	}

	game->Shutdown();
	delete game;

	graphics->Exit();
}

// OpenGL helper functions
void _CheckGL( const char* f, int l )
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const char* errStr = "UNKNOWN ERROR";
		if (error == 0x500) errStr = "INVALID ENUM";
		else if (error == 0x502) errStr = "INVALID OPERATION";
		else if (error == 0x501) errStr = "INVALID VALUE";
		else if (error == 0x506) errStr = "INVALID FRAMEBUFFER OPERATION";

		std::cerr << "GL error " << error << "; " << errStr << "\n" << f << "\n" << l << std::endl;
		assert(false);
	}
}
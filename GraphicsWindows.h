#pragma once
#include "Graphics.h"

struct GLFWwindow;

class GraphicsWindows :
    public Graphics
{
public:
	void Init(int width, int height);
	void NewFrame();
	void Render();
	void Exit();

	GLFWwindow* mWindow{};
};


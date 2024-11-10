#include "precomp.h"
#include "InputManager.h"

#ifdef PLATFORM_LINUX
#include "GraphicsLinux.h"

#include "linux/input-event-codes.h"
#include <linux/input.h>
#elif PLATFORM_WINDOWS
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "GraphicsWindows.h"
#endif // PLATFORM_LINUX

#include "Input.h"
#include "SavedData.h"

Framework::InputManager::InputManager() = default;
Framework::InputManager::~InputManager() = default;

#ifdef PLATFORM_LINUX
void Framework::InputManager::Init(const Graphics* graphicsHandle)
{
	mGraphicLinux = dynamic_cast<const GraphicsLinux*>(graphicsHandle);
}

constexpr KeySym Framework::InputManager::ToKeySym(Framework::InputId id)
{
	switch (id)
	{
		case InputId::A: return XK_a;
		case InputId::B: return XK_b;
		case InputId::C: return XK_c;
		case InputId::D: return XK_d;
		case InputId::E: return XK_e;
		case InputId::F: return XK_f;
		case InputId::G: return XK_g;
		case InputId::H: return XK_h;
		case InputId::I: return XK_i;
		case InputId::J: return XK_j;
		case InputId::K: return XK_k;
		case InputId::L: return XK_l;
		case InputId::M: return XK_m;
		case InputId::N: return XK_n;
		case InputId::O: return XK_o;
		case InputId::P: return XK_p;
		case InputId::Q: return XK_q;
		case InputId::R: return XK_r;
		case InputId::S: return XK_s;
		case InputId::T: return XK_t;
		case InputId::U: return XK_u;
		case InputId::V: return XK_v;
		case InputId::W: return XK_w;
		case InputId::X: return XK_x;
		case InputId::Y: return XK_y;
		case InputId::Z: return XK_z;
		case InputId::NORMAL_0: return XK_0;
		case InputId::NORMAL_1: return XK_1;
		case InputId::NORMAL_2: return XK_2;
		case InputId::NORMAL_3: return XK_3;
		case InputId::NORMAL_4: return XK_4;
		case InputId::NORMAL_5: return XK_5;
		case InputId::NORMAL_6: return XK_6;
		case InputId::NORMAL_7: return XK_7;
		case InputId::NORMAL_8: return XK_8;
		case InputId::NORMAL_9: return XK_9;
		case InputId::SPACE: return XK_space;
		case InputId::LEFT_SHIFT:return XK_Shift_L;
		case InputId::LEFT_CTRL: return XK_Control_L;
		case InputId::ARROW_UP: return XK_Up;
		case InputId::ARROW_LEFT: return XK_Left;
		case InputId::ARROW_DOWN: return XK_Down;
		case InputId::ARROW_RIGHT: return XK_Right;
		case InputId::DOT: return XK_period;
		case InputId::MINUS: return XK_minus;
		case InputId::BACK_SPACE: return XK_BackSpace;
		case InputId::ESC: return XK_Escape;
		case InputId::NONE: assert(false);
		default: return 0;
	}
}

constexpr ImGuiKey Framework::InputManager::ToImGuiKey(const Framework::InputId id)
{
	switch (id)
	{
	case InputId::A: return ImGuiKey_A;
	case InputId::B: return ImGuiKey_B;
	case InputId::C: return ImGuiKey_C;
	case InputId::D: return ImGuiKey_D;
	case InputId::E: return ImGuiKey_E;
	case InputId::F: return ImGuiKey_F;
	case InputId::G: return ImGuiKey_G;
	case InputId::H: return ImGuiKey_H;
	case InputId::I: return ImGuiKey_I;
	case InputId::J: return ImGuiKey_J;
	case InputId::K: return ImGuiKey_K;
	case InputId::L: return ImGuiKey_L;
	case InputId::M: return ImGuiKey_M;
	case InputId::N: return ImGuiKey_N;
	case InputId::O: return ImGuiKey_O;
	case InputId::P: return ImGuiKey_P;
	case InputId::Q: return ImGuiKey_Q;
	case InputId::R: return ImGuiKey_R;
	case InputId::S: return ImGuiKey_S;
	case InputId::T: return ImGuiKey_T;
	case InputId::U: return ImGuiKey_U;
	case InputId::V: return ImGuiKey_V;
	case InputId::W: return ImGuiKey_W;
	case InputId::X: return ImGuiKey_X;
	case InputId::Y: return ImGuiKey_Y;
	case InputId::Z: return ImGuiKey_Z;
	case InputId::NORMAL_0: return ImGuiKey_0;
	case InputId::NORMAL_1: return ImGuiKey_1;
	case InputId::NORMAL_2: return ImGuiKey_2;
	case InputId::NORMAL_3: return ImGuiKey_3;
	case InputId::NORMAL_4: return ImGuiKey_4;
	case InputId::NORMAL_5: return ImGuiKey_5;
	case InputId::NORMAL_6: return ImGuiKey_6;
	case InputId::NORMAL_7: return ImGuiKey_7;
	case InputId::NORMAL_8: return ImGuiKey_8;
	case InputId::NORMAL_9: return ImGuiKey_9;
	case InputId::SPACE: return ImGuiKey_Space;
	case InputId::LEFT_SHIFT:return ImGuiKey_LeftShift;
	case InputId::LEFT_CTRL: return ImGuiKey_LeftCtrl;
	case InputId::ARROW_UP: return ImGuiKey_UpArrow;
	case InputId::ARROW_LEFT: return ImGuiKey_LeftArrow;
	case InputId::ARROW_DOWN: return ImGuiKey_DownArrow;
	case InputId::ARROW_RIGHT: return ImGuiKey_RightArrow;
	case InputId::DOT: return ImGuiKey_Period;
	case InputId::MINUS: return ImGuiKey_Minus;
	case InputId::BACK_SPACE: return ImGuiKey_Backspace;
	case InputId::ESC: return ImGuiKey_Escape;
	default: return ImGuiKey_None;
	}
}

#elif PLATFORM_WINDOWS

void Framework::InputManager::MouseWheel(float f)
{
	mouseWheelChange = static_cast<int>(f);
}

void Framework::InputManager::MouseMove(int x, int y)
{
	mousePos = { x, y };
}

constexpr Framework::InputId Framework::InputManager::GLFWKeyToInputID(uint glfwNum, bool isMouseButton)
{
	if (isMouseButton)
	{
		switch (glfwNum)
		{
		case GLFW_MOUSE_BUTTON_1: return InputId::MOUSE_1;
		case GLFW_MOUSE_BUTTON_2: return InputId::MOUSE_2;
		default: return InputId::NONE;
		}
	}
	else
	{
		switch (glfwNum)
		{
		case GLFW_KEY_A: return InputId::A;
		case GLFW_KEY_B: return InputId::B;
		case GLFW_KEY_C: return InputId::C;
		case GLFW_KEY_D: return InputId::D;
		case GLFW_KEY_E: return InputId::E;
		case GLFW_KEY_F: return InputId::F;
		case GLFW_KEY_G: return InputId::G;
		case GLFW_KEY_H: return InputId::H;
		case GLFW_KEY_I: return InputId::I;
		case GLFW_KEY_J: return InputId::J;
		case GLFW_KEY_K: return InputId::K;
		case GLFW_KEY_L: return InputId::L;
		case GLFW_KEY_M: return InputId::M;
		case GLFW_KEY_N: return InputId::N;
		case GLFW_KEY_O: return InputId::O;
		case GLFW_KEY_P: return InputId::P;
		case GLFW_KEY_Q: return InputId::Q;
		case GLFW_KEY_R: return InputId::R;
		case GLFW_KEY_S: return InputId::S;
		case GLFW_KEY_T: return InputId::T;
		case GLFW_KEY_U: return InputId::U;
		case GLFW_KEY_V: return InputId::V;
		case GLFW_KEY_W: return InputId::W;
		case GLFW_KEY_X: return InputId::X;
		case GLFW_KEY_Y: return InputId::Y;
		case GLFW_KEY_Z: return InputId::Z;
		case GLFW_KEY_0: return InputId::NORMAL_0;
		case GLFW_KEY_1: return InputId::NORMAL_1;
		case GLFW_KEY_2: return InputId::NORMAL_2;
		case GLFW_KEY_3: return InputId::NORMAL_3;
		case GLFW_KEY_4: return InputId::NORMAL_4;
		case GLFW_KEY_5: return InputId::NORMAL_5;
		case GLFW_KEY_6: return InputId::NORMAL_6;
		case GLFW_KEY_7: return InputId::NORMAL_7;
		case GLFW_KEY_8: return InputId::NORMAL_8;
		case GLFW_KEY_9: return InputId::NORMAL_9;
		case GLFW_KEY_SPACE: return InputId::SPACE;
		case GLFW_KEY_LEFT_SHIFT: return InputId::LEFT_SHIFT;
		case GLFW_KEY_LEFT_CONTROL: return InputId::LEFT_CTRL;
		case GLFW_KEY_UP: return InputId::ARROW_UP;
		case GLFW_KEY_DOWN: return InputId::ARROW_DOWN;
		case GLFW_KEY_LEFT: return InputId::ARROW_LEFT;
		case GLFW_KEY_RIGHT: return InputId::ARROW_RIGHT;
		case GLFW_KEY_MINUS: return InputId::MINUS;
		case GLFW_KEY_ESCAPE: return InputId::ESC;
		case GLFW_KEY_PERIOD: return InputId::DOT;
		default: return InputId::NONE;
		}
	}
}

void KeyEventCallback(GLFWwindow*, int key, int, int action, int)
{
	if (action == GLFW_PRESS)
	{
		const Framework::InputId id = Framework::InputManager::GLFWKeyToInputID(key, false);
		Framework::InputManager::Inst().UpdateInputState(id, Framework::InputManager::Interaction::pressed);
	}
	else if (action == GLFW_RELEASE)
	{
		const Framework::InputId id = Framework::InputManager::GLFWKeyToInputID(key, false);
		Framework::InputManager::Inst().UpdateInputState(id, Framework::InputManager::Interaction::released);
	}
}
void MouseButtonCallback(GLFWwindow*, int button, int action, int)
{
	if (action == GLFW_PRESS)
	{
		const Framework::InputId id = Framework::InputManager::GLFWKeyToInputID(button, true);
		Framework::InputManager::Inst().UpdateInputState(id, Framework::InputManager::Interaction::pressed);
	}
	else if (action == GLFW_RELEASE)
	{
		const Framework::InputId id = Framework::InputManager::GLFWKeyToInputID(button, true);
		Framework::InputManager::Inst().UpdateInputState(id, Framework::InputManager::Interaction::released);
	}
}
void MouseScrollCallback(GLFWwindow*, double, double y)
{
	Framework::InputManager::Inst().MouseWheel(static_cast<float>(y));
}
void MousePosCallback(GLFWwindow*, double x, double y)
{
	Framework::InputManager::Inst().MouseMove(static_cast<int>(x), static_cast<int>(y));
}

void Framework::InputManager::Init(const Graphics* graphicsHandle)
{
	mGraphicsWindows = static_cast<const GraphicsWindows*>(graphicsHandle);

	glfwSetKeyCallback(mGraphicsWindows->mWindow, KeyEventCallback);
	glfwSetMouseButtonCallback(mGraphicsWindows->mWindow, MouseButtonCallback);
	glfwSetScrollCallback(mGraphicsWindows->mWindow, MouseScrollCallback);
	glfwSetCursorPosCallback(mGraphicsWindows->mWindow, MousePosCallback);
}
#endif // PLATFORM_LINUX

void Framework::InputManager::NewFrame()
{
	while (!interactedWith.empty())
	{
		InputId id = interactedWith.front();
		interactedWith.pop();
		inputs[static_cast<size_t>(id)].ResetForNextFrame();
	}
	mouseWheelChange = 0;
	
#ifdef PLATFORM_LINUX
	ImGuiIO& io = ImGui::GetIO();
	
	Display* dpy = XOpenDisplay(":0");
	char keys_return[32];
	XQueryKeymap(dpy, keys_return);

	// Keyboard input
	for (InputId id = InputId::FIRST_KEY; id <= InputId::LAST_KEY; id = static_cast<InputId>(static_cast<size_t>(id) + 1))
	{
		const KeyCode kc2 = XKeysymToKeycode(dpy, ToKeySym(id));
		const bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));

		if (isPressed)
		{
			if (!GetInput(id).held)
			{
				UpdateInputState(id, Interaction::pressed);
				io.AddInputCharacter(ToText(id));
				io.AddKeyEvent(ToImGuiKey(id), true);
			}
		}
		else if (GetInput(id).held)
		{
			UpdateInputState(id, Interaction::released);
			io.AddKeyEvent(ToImGuiKey(id), false);
		}
	}
	XCloseDisplay(dpy);

	//// we need this to collect the mouse data
	Window window_returned;
	int root_x;
	int root_y;
	int win_x;
	int win_y;
	uint mask_return;

	bool result = XQueryPointer(
		mGraphicLinux->x_display,
		mGraphicLinux->win,
		&window_returned,
		&window_returned,
		&root_x,
		&root_y,
		&win_x,
		&win_y,
		&mask_return);

	if (!result)
	{
		LOGERROR("XQueryPointer failed!");
		return;
	}

	io.AddMousePosEvent(static_cast<float>(root_x), static_cast<float>(root_y));
	mousePos = { root_x, root_y };
	bool mousesHeldNow[2];
	mousesHeldNow[0] = mask_return & (1 << 8); // left
	mousesHeldNow[1] = mask_return & (1 << 10); // right

	for (InputId id = InputId::FIRST_MOUSE; id < InputId::LAST_MOUSE; id = static_cast<InputId>(static_cast<size_t>(id) + 1))
	{
		Input input = GetInput(id);
		int mouseButton = static_cast<int>(id) - static_cast<int>(InputId::FIRST_MOUSE);

		if (!input.held
			&& mousesHeldNow[mouseButton])
		{
			UpdateInputState(id, Interaction::pressed);
			io.AddMouseButtonEvent(mouseButton, true);
		}
		else if (input.held
			&& !mousesHeldNow[mouseButton])
		{
			UpdateInputState(id, Interaction::released);
			io.AddMouseButtonEvent(mouseButton, false);
		}
	}
#elif PLATFORM_WINDOWS
	glfwPollEvents(); // Don't you just love glfw
#endif // PLATFORM_LINUX
}

constexpr char Framework::InputManager::ToText(InputId id)
{
	switch (id)
	{
	case InputId::NORMAL_0: return '0';
	case InputId::NORMAL_1: return '1';
	case InputId::NORMAL_2: return '2';
	case InputId::NORMAL_3: return '3';
	case InputId::NORMAL_4: return '4';
	case InputId::NORMAL_5: return '5';
	case InputId::NORMAL_6: return '6';
	case InputId::NORMAL_7: return '7';
	case InputId::NORMAL_8: return '8';
	case InputId::NORMAL_9: return '9';
	case InputId::DOT: return '.';
	case InputId::A: return 'a';
	case InputId::B: return 'b';
	case InputId::C: return 'c';
	case InputId::D: return 'd';
	case InputId::E: return 'e';
	case InputId::F: return 'f';
	case InputId::G: return 'g';
	case InputId::H: return 'h';
	case InputId::I: return 'i';
	case InputId::J: return 'j';
	case InputId::K: return 'k';
	case InputId::L: return 'l';
	case InputId::M: return 'm';
	case InputId::N: return 'n';
	case InputId::O: return 'o';
	case InputId::P: return 'p';
	case InputId::Q: return 'q';
	case InputId::R: return 'r';
	case InputId::S: return 's';
	case InputId::T: return 't';
	case InputId::U: return 'u';
	case InputId::V: return 'v';
	case InputId::W: return 'w';
	case InputId::X: return 'x';
	case InputId::Y: return 'y';
	case InputId::Z: return 'z';
	case InputId::MINUS: return '-';
	case InputId::SPACE: return ' ';
	default: return '?';
	}
}

void Framework::InputManager::UpdateInputState(InputId id, Interaction interaction)
{
	if (id == InputId::NONE)
	{
		//LOGWARNING("Key not supported!");
		return;
	}

	size_t index = static_cast<size_t>(id);

	assert(index >= 0
		&& index < inputs.size());

	Input& inp = inputs[index];
	interactedWith.push(id);

	if (interaction == Interaction::pressed)
	{
		inp.Down();
		//LOGMESSAGE('\'' << ToText(id) << "\' pressed.");
	}
	else
	{
		inp.Up();
	}
}
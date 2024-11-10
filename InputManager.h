#pragma once
#include "Singleton.h"
#include "Input.h"


class InvadersInput;
class GraphicsLinux;
class GraphicsWindows;
class Graphics;

namespace Framework
{
	class SavedData;

	class InputManager :
		public Singleton<InputManager>
	{
		friend Singleton;
		InputManager();
		~InputManager();

	public:
#ifdef PLATFORM_LINUX
		void Init(const Graphics* graphicsHandle);
#elif PLATFORM_WINDOWS
		void Init(const Graphics* graphicsHandle);
#endif // PLATFORM_LINUX

		void NewFrame();

		static inline glm::ivec2 GetMousePos() { return Inst().mousePos; };

		static inline int GetMouseWheel() { return Inst().mouseWheelChange; }

		static inline const Input& GetInput(InputId inputID) { return Inst().IGetInput(inputID); }

		inline Input& IGetInput(InputId inputID) { return inputs[static_cast<uint>(inputID)]; }
		template <typename T> inline Input* IGetInput(T notAnID) = delete;

		//void LoadKeyBindingsFromFile(bool loadDefaultValues = false);

		enum class Interaction { pressed, released };
		void UpdateInputState(InputId keyCode, Interaction interaction);

		void MouseWheel(float f);
		void MouseMove(int x, int y);

#ifdef PLATFORM_LINUX
		static constexpr KeySym ToKeySym(const Framework::InputId id);
		static constexpr ImGuiKey ToImGuiKey(const Framework::InputId id);
#elif PLATFORM_WINDOWS
		static constexpr InputId GLFWKeyToInputID(uint glfwNum, bool isMouseButton);
#endif
		static constexpr char ToText(InputId id);
	private:


#ifdef PLATFORM_LINUX
		const GraphicsLinux* mGraphicLinux{};
#elif PLATFORM_WINDOWS
		const GraphicsWindows* mGraphicsWindows;
#endif // PLATFORM_LINUX

		glm::ivec2 mousePos{};
		int mouseWheelChange{};

		// All the inputs that were pressed or released this frame, and which were bound to atleast one keybinding. Can be used to update the pressed and released state.
		std::queue<InputId> interactedWith{};
		std::array<Input, static_cast<size_t>(InputId::INPUTID_LAST) + 1> inputs{};
	};
}

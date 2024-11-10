#pragma once

namespace Framework
{
	class Input
	{
	public:
		Input() = default;

		void Down();
		void Up();

		void ResetForNextFrame();

		bool down = false;
		bool up = false;
		bool held = false;
	};

	// The index of an Input class.
	enum class InputId : int
	{
		NONE = -1,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		NORMAL_0,
		NORMAL_1,
		NORMAL_2,
		NORMAL_3,
		NORMAL_4,
		NORMAL_5,
		NORMAL_6,
		NORMAL_7,
		NORMAL_8,
		NORMAL_9,
		SPACE,
		LEFT_SHIFT,
		LEFT_CTRL,
		ARROW_UP,
		ARROW_LEFT,
		ARROW_DOWN,
		ARROW_RIGHT,
		DOT,
		MINUS,
		BACK_SPACE,
		ESC,
		MOUSE_LEFT,
		MOUSE_RIGHT,

		INPUTID_LAST,
		LAST_KEY = ESC,
		LAST_MOUSE = INPUTID_LAST,

		FIRST_KEY = A,
		FIRST_MOUSE = MOUSE_LEFT,

		MOUSE_1 = MOUSE_LEFT,
		MOUSE_2 = MOUSE_RIGHT,
	};
}
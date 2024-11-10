#include "precomp.h"
#include "Input.h"

void Framework::Input::Down()
{
	if (!held)
	{
		down = true;
	}
	held = true;
}

void Framework::Input::Up()
{
	held = false;
	up = true;
}

void Framework::Input::ResetForNextFrame()
{
	up = false;
	down = false;
}


#pragma once

class Graphics
{
public:
	Graphics();
	virtual ~Graphics();

	virtual void Init(int width, int height) = 0;
	virtual void NewFrame() = 0;
	virtual void Render() = 0;
	virtual void Exit() = 0;


};
#pragma once
#include "Graphics.h"

// general info on the state of the EGL/DispmanX/X11 screen
typedef struct Target_State
{
	uint32_t width;
	uint32_t height;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	EGLNativeWindowType  nativewindow;
} Target_State;


// define what kind of EGL config we want, we can add more options but the more we add the less likely it might be available across systems
static const EGLint attribute_list[] =
{
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_DEPTH_SIZE,	8,
	EGL_SURFACE_TYPE,
	EGL_WINDOW_BIT,
#ifdef GLES3	
	EGL_CONFORMANT,
	EGL_OPENGL_ES3_BIT_KHR,
#endif	
	//be aware, some emulated Mesa OpenGLES2.0 drivers, can choke on these two lines
	// and on Pi4 ES3.x and X11 it really bites on the FPS. How many will notice this and report the difference?
	EGL_SAMPLE_BUFFERS, 1,		// if you want anti alias at variable fps cost
	EGL_SAMPLES, 4,				//keep these 2lines, especially useful for lower resolution

	EGL_NONE
};

static const EGLint GiveMeGLES3[] = {
	EGL_CONTEXT_MAJOR_VERSION_KHR,
	3,
	EGL_CONTEXT_MINOR_VERSION_KHR,
	1,
	EGL_NONE,
	EGL_NONE
};

class GraphicsLinux :
    public Graphics
{
public:
    void Init(int width, int height) override;
	void NewFrame() override;
	void Render() override;
	void Exit() override;

	Target_State mState{};

	// info needed for X11 screen info, not all used initally.
	short                   original_rate;
	Rotation                original_rotation;
	SizeID                  original_size_id;

	XRRScreenSize* xrrs;
	XRRScreenConfiguration* conf;

	// we can store all the possible resolutions here		
	short possible_frequencies[64][64]; // a big list of details
	int num_rates; // how many possible rates
	int num_sizes; // how many possible sizes
	Display* x_display;
	Window win;
	Window root;

	EGLDisplay egldisplay;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint num_config;
};


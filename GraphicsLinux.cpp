#include "precomp.h"
#include "GraphicsLinux.h"

void GraphicsLinux::Init(int width, int height)
{
	// InitDisplay()
	x_display = XOpenDisplay(NULL);
	assert(x_display != NULL
		&& "Failed to open an XDisplay");

	root = DefaultRootWindow(x_display);

	// esInitContext()
	memset(&mState, 0, sizeof(Target_State));

	// InitOGL
	XSetWindowAttributes swa;
	XSetWindowAttributes  xattr;
	Atom wm_state;
	XWMHints hints;
	XEvent xev;

	mState.width = width;
	mState.height = height;

	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;

	root = DefaultRootWindow(x_display);

	swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask | KeyReleaseMask;
	swa.background_pixmap = None;
	swa.background_pixel = 0;
	swa.border_pixel = 0;
	swa.override_redirect = true;

	win = XCreateWindow(
		x_display,
		root,
		0,		// puts it at the top left of the screen
		0,
		mState.width,	//set size  
		mState.height,
		0,
		CopyFromParent,
		InputOutput,
		CopyFromParent,
		CWEventMask,
		&swa);

	assert(win != 0);

	mState.nativewindow = (EGLNativeWindowType)win;

	XSelectInput(x_display, win, KeyPressMask | KeyReleaseMask);
	xattr.override_redirect = TRUE;
	XChangeWindowAttributes(x_display, win, CWOverrideRedirect, &xattr);

	hints.input = TRUE;
	hints.flags = InputHint;
	XSetWMHints(x_display, win, &hints);

	char* title = (char*)"x11 Demo";
	// make the window visible on the screen
	XMapWindow(x_display, win);
	XStoreName(x_display, win, title);

	// get identifiers for the provided atom name strings
	wm_state = XInternAtom(x_display, "_NET_WM_STATE", FALSE);

	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = FALSE;
	XSendEvent(
		x_display,
		DefaultRootWindow(x_display),
		FALSE,
		SubstructureNotifyMask,
		&xev);
	// end of xdisplay

	// Get EGLDisplay	
	egldisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);       //eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(egldisplay != EGL_NO_DISPLAY);

	bool succes;
	// Initialize EGL
	succes = eglInitialize(egldisplay, &majorVersion, &minorVersion);
	assert(succes);

	// Get configs
	succes = eglGetConfigs(egldisplay, NULL, 0, &numConfigs);
	assert(succes);

	// Choose config
	succes = eglChooseConfig(egldisplay, attribute_list, &config, 1, &numConfigs);
	assert(succes);

	// Create a GL context
	context = eglCreateContext(egldisplay, config, NULL, GiveMeGLES3);
	assert(context != EGL_NO_CONTEXT);

	// Create a surface
	surface = eglCreateWindowSurface(egldisplay, config, mState.nativewindow, NULL);     // this fails with a segmentation error?
	assert(surface != EGL_NO_SURFACE);

	succes = eglMakeCurrent(egldisplay, surface, surface, context);
	assert(succes);

	mState.display = egldisplay;
	mState.surface = surface;
	mState.context = context;

	eglSwapInterval(egldisplay, 01);        // 1 to lock speed to 60fps (assuming we are able to maintain it), 0 for immediate swap (may cause tearing) which will indicate actual frame rate
	// on xu4 this seems to have no effect

	XSetInputFocus(x_display, win, RevertToNone, CurrentTime);

	// For if you want to print some extra info
	if (false
		&& succes)
	{
		int t; // a dum,ing variable to extra some useful data

		printf("This SBC supports version %i.%i of EGL\n", majorVersion, minorVersion);
		printf("This GPU supplied by  :%s\n", glGetString(GL_VENDOR));
		printf("This GPU supports     :%s\n", glGetString(GL_VERSION));
		printf("This GPU Renders with :%s\n", glGetString(GL_RENDERER));
		printf("This GPU supports     :%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));


		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &t);
		printf("This GPU MaxTexSize is    :%i\n", t);
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &t);
		printf("This GPU supports %i Texture units\n", t); // pi4 16

		printf("This GPU supports these extensions	:%s\n", glGetString(GL_EXTENSIONS));
	}
	glViewport(0, 0, mState.width, mState.height);
}

void GraphicsLinux::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();
}

void GraphicsLinux::Render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glFlush();
	eglSwapBuffers(mState.display, mState.surface);
}

void GraphicsLinux::Exit()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();
}

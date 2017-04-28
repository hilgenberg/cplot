#pragma once
#include <string>

//#include <GL/glew.h>
//#include <GL/glxew.h>
//#include <GL/gl.h>
#include <GL/glx.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#undef Complex

struct XWindow
{
	XWindow(const std::string &title, char *display_name = NULL);
	virtual ~XWindow();

	bool handle(XEvent &e);

	void swap_buffers() const;

	operator bool() const{ return !closed; }

	Display *display;
	int screen_num;

	Window rootWindow;
	Window window;
	unsigned w, h;
	bool glewOK;    // if one process opens several windows, this must go elsewhere!
	int  accum;     // accumulation buffer size
	int  modifiers; // current modifier state

	GLXContext context;

	virtual void reshape() = 0;
	virtual bool handle_key(KeySym key, const char *s, bool release) = 0;
	//virtual bool handle_click(int button, int mx, int my) = 0;
	virtual bool handle_scroll(double dx, double dy, bool discrete) = 0;
	virtual bool handle_drag(int buttons, double dx, double dy) = 0;
	virtual bool handle_other(XEvent &e) = 0;

protected:
	Atom XA_WM_PROTOCOLS;
	Atom XA_WM_DELETE_WINDOW;

	bool closed; // should window close?
	int mx, my; // drag start mouse position

	bool xi2; // XInput2 available?
	int  xi2_opcode; // XInput2 cookie
	bool handle(XIDeviceEvent &e);
};


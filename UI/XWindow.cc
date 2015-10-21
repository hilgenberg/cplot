#include <GL/glew.h>
#include <GL/glxew.h>
#include "XWindow.h"
#include <GL/gl.h>
#include "../Utility/System.h"
#include <memory>
#include <vector>
#include <map>
#include <cassert>
#include <limits>
#include <cmath>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#define ScrollUpButton    5
#define ScrollDownButton  4
#define ScrollLeftButton  7
#define ScrollRightButton 6

#define DNAN std::numeric_limits<double>::quiet_NaN()

#ifdef DEBUG
//#define EVDEBUG
#endif

#ifdef EVDEBUG
#define DP(s, ...) fprintf(stderr, s "\n", ##__VA_ARGS__)
#else
#define DP(...)
#endif

XWindow::XWindow(const std::string &title, char *display_name)
: display(NULL), screen_num(0)
, rootWindow(0), window(0)
, w(0), h(0)
, closed(false)
, glewOK(false)
{
	display = XOpenDisplay(display_name);
	if (!display) throw std::runtime_error("can't connect to X server.");
	std::unique_ptr<Display, int(*)(Display*)> cdp(display, XCloseDisplay);

	screen_num = DefaultScreen(display);
	rootWindow = RootWindowOfScreen(ScreenOfDisplay(display, screen_num));

	Atom XA_SWM_VROOT = XInternAtom(display, "__SWM_VROOT", False);
	Window r, p, *c = NULL; unsigned nc;
	if (XQueryTree(display, rootWindow, &r, &p, &c, &nc) && c)
	{
		for (unsigned i = 0; i < nc; ++i)
		{
			Atom type; int format; unsigned long n, m;
			Window *newRoot = NULL;

			if (XGetWindowProperty(display, c[i], XA_SWM_VROOT, 0, 1, False, XA_WINDOW,
			      &type, &format, &n, &m, (unsigned char **)&newRoot) == Success && newRoot)
			{
				rootWindow = *newRoot;
				break;
			}
		}
		XFree(c);
	}

	int attributes[] =
	{
		GLX_RGBA,
		GLX_RED_SIZE,   8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE,  8,
		GLX_DEPTH_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DOUBLEBUFFER,
		GLX_ACCUM_RED_SIZE,   16,
		GLX_ACCUM_GREEN_SIZE, 16,
		GLX_ACCUM_BLUE_SIZE,  16,
		0
	};

	XVisualInfo *vi = glXChooseVisual(display, screen_num, attributes);
	if (!vi)
	{
		attributes[12] = 0; // try without accumulation buffer
		vi = glXChooseVisual(display, screen_num, attributes);
	}
	if (!vi)
	{
		attributes[11] = 0; // try without double buffering even
		vi = glXChooseVisual(display, screen_num, attributes);
	}
	if (!vi) throw std::runtime_error("can't open GL visual.");
	std::unique_ptr<XVisualInfo, int(*)(void*)> vip(vi, XFree);

	int v;
	glXGetConfig(display, vi, GLX_ACCUM_RED_SIZE,   &v); accum = v;
	glXGetConfig(display, vi, GLX_ACCUM_GREEN_SIZE, &v); if (v < accum) accum = v;
	glXGetConfig(display, vi, GLX_ACCUM_BLUE_SIZE,  &v); if (v < accum) accum = v;
	glXGetConfig(display, vi, GLX_ACCUM_ALPHA_SIZE, &v); if (v < accum) accum = v;

	xi2 = false;
	int event, error;
	if (XQueryExtension(display, "XInputExtension", &xi2_opcode, &event, &error))
	{
		int major = 2, minor = 0;
		if (XIQueryVersion(display, &major, &minor) != BadRequest)
		{
			xi2 = true;
			DP("XI2 %d.%d available.", major, minor);
		}
	}

	XSetWindowAttributes swa;
	swa.colormap     = XCreateColormap(display, rootWindow, vi->visual, AllocNone);
	swa.border_pixel = swa.background_pixel = swa.backing_pixel = BlackPixel(display, screen_num);
	swa.event_mask   = KeyPressMask | KeyReleaseMask | VisibilityChangeMask | StructureNotifyMask | FocusChangeMask;
	if (!xi2) swa.event_mask |= ButtonPressMask | ButtonReleaseMask | Button1MotionMask | Button2MotionMask | Button3MotionMask | ButtonMotionMask;

	w = DisplayWidth (display, screen_num) / 2;
	h = DisplayHeight(display, screen_num) / 2;

	window = XCreateWindow(display, rootWindow, 0, 0, w, h, 0, vi->depth, InputOutput, vi->visual,
			CWBorderPixel | CWBackPixel | CWBackingPixel | CWColormap | CWEventMask, &swa);
	if (!window) throw std::runtime_error("can't create window.");
	std::unique_ptr<Window, std::function<void(Window*)>> 
		wdp(&window, [this](Window *w){ XDestroyWindow(display, *w);} );

	if (xi2)
	{
		XIEventMask em;
		unsigned char mask[XIMaskLen(XI_LASTEVENT)] = { 0 };
		em.deviceid = XIAllDevices;
		em.mask_len = sizeof(mask);
		em.mask = mask;
		XISetMask(mask, XI_ButtonPress);
		XISetMask(mask, XI_ButtonRelease);
		XISetMask(mask, XI_Motion);
		//XISetMask(mask, XI_KeyPress);
		//XISetMask(mask, XI_KeyRelease);
		XISelectEvents(display, window, &em, 1);
	}

	XSizeHints hints;
	hints.flags  = USSize;
	hints.width  = w;
	hints.height = h;

	XWMHints wmHints;
	wmHints.flags = InputHint;
	wmHints.input = True;

	XmbSetWMProperties(display, window, title.c_str(), NULL, argv, argc, &hints, &wmHints, NULL);

	context = glXCreateContext(display, vi, 0, GL_TRUE);
	if (!context) throw std::runtime_error("can't open GLX context.");
	if (!glXMakeCurrent(display, window, context)) throw std::runtime_error("can't set GL context.");

	XMapWindow(display, window);
	
	glewOK = (glewInit() == GLEW_OK);
	int vsync = 1;
	if (vsync && glewOK && GLXEW_SGI_swap_control) glXSwapIntervalSGI(vsync);

	XA_WM_PROTOCOLS     = XInternAtom(display, "WM_PROTOCOLS",     False);
	XA_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &XA_WM_DELETE_WINDOW, 1);
#if 0
	Atom proto[2] = {
		XA_WM_DELETE_WINDOW,
		XInternAtom(display, "WM_TAKE_FOCUS",    False)
	};
	XSetWMProtocols(display, window, proto, 2);
#endif
	cdp.release();
	wdp.release();
	//~vip is needed
}

XWindow::~XWindow()
{
	if (display)
	{
		if (window) XDestroyWindow(display, window);
		XCloseDisplay(display);
	}
}

void XWindow::swap_buffers() const
{
	glXSwapBuffers(display, window);
}

struct Device
{
	struct Axis
	{
		Axis() : abs(false), min(0.0), max(0.0), value(DNAN), valid(false), sx(false), sy(false), unit(0.0){ }
		bool abs; // absolute or relative values?
		double min, max; // range
		double value; // range and last seen value
		bool sx, sy; // scrolls horizontally, vertically
		double unit; // scroll unit
		bool scrollbuttons; // scrolling sends legacy button events
		bool valid; // these live in a vector and filler entries are set to invalid
	};

	Device() : nb(0), nk(0), valid(false), ignore_scrollbuttons(false){ }
	
	int nb, nk; // number of buttons, keycodes
	bool valid; // device info could be read ok?
	bool ignore_scrollbuttons; // set to true if we ever see real scroll events from this device
	std::vector<Axis> axis;
};
static std::map<int, Device> devices; // id -> info

static Device &devinfo(int deviceid, Display *display)
{
	auto i = devices.find(deviceid);
	if (i != devices.end()) return i->second;

	Device &dev = devices[deviceid];

	int nd = 0;
	XIDeviceInfo *ds = XIQueryDevice(display, deviceid, &nd);
	assert(nd == 1);
	for (int i = 0; i < nd; ++i)
	{
		XIDeviceInfo &d = ds[i];
		#ifdef EVDEBUG
		const char *t = "Unknown";
		switch (d.use)
		{
			case XIMasterPointer:  t = "master pointer";  break;
			case XIMasterKeyboard: t = "master keyboard"; break;
			case XISlavePointer:   t = "slave pointer";   break;
			case XISlaveKeyboard:  t = "slave keyboard";  break;
			case XIFloatingSlave:  t = "floating slave";  break;
		}
		DP("Device %s (id: %d) is a %s", d.name, d.deviceid, t);
		DP(" -- attached to/paired with %d", d.attachment);
		#endif

		for (int j = 0; j < d.num_classes; ++j)
		{
			XIAnyClassInfo *ci = d.classes[j];
			if (!ci){ assert(false); continue; }
			dev.valid = true;
			switch (ci->type)
			{
				case XIKeyClass:
				{
					XIKeyClassInfo &c = *(XIKeyClassInfo*)ci;
					dev.nk = c.num_keycodes;
					DP(" -- %d keys", dev.nk);
					break;
				}
				case XIButtonClass:
				{
					XIButtonClassInfo &c = *(XIButtonClassInfo*)ci;
					dev.nb = c.num_buttons;
					DP(" -- %d buttons", dev.nb);
					break;
				}
				case XIValuatorClass:
				{
					XIValuatorClassInfo &c = *(XIValuatorClassInfo*)ci;
					int n = c.number;
					if (n < 0){ assert(false); continue; }
					if (n >= (int)dev.axis.size()) dev.axis.resize(n+1);
					Device::Axis &a = dev.axis[n];
					a.min = c.min; a.max = c.max;
					a.abs = (c.mode == XIModeRelative);
					//a.value = c.value; -- leave at NAN
					a.valid = true;

					// c.mode fails a lot, try the label too
					char *name = XGetAtomName(display, c.label);
					if (!a.abs && !strncasecmp(name, "abs ", 4)) a.abs = true;
					//if ( a.abs && !strncasecmp(name, "rel ", 4)) a.abs = false;
					
					DP(" -- v%d %s (%d) [%g,%g]=%g %d->%s", n, name, (int)c.label, c.min, c.max, c.value, c.mode, a.abs ? "abs" : "rel");
					if (name) XFree(name);
					break;
				}
				case XIScrollClass:
				{
					XIScrollClassInfo &c = *(XIScrollClassInfo*)ci;
					int n = c.number;
					if (n < 0){ assert(false); continue; }
					if (n >= (int)dev.axis.size()) dev.axis.resize(n+1);
					Device::Axis &a = dev.axis[n];
					(c.scroll_type==XIScrollTypeVertical ? a.sy : a.sx) = true;
					a.unit = c.increment / -16.0;
					a.scrollbuttons = !(c.flags & XIScrollFlagNoEmulation);
					DP(" -- s%d %c (%g) %s", n, a.sy ? 'y' : 'x', a.unit, a.scrollbuttons ? "emu" : "noemu");
					break;
				}
				case XITouchClass:
				{
					#ifdef EVDEBUG
					XITouchClassInfo &c = *(XITouchClassInfo*)ci;
					DP(" -- touch %d %s", c.num_touches, c.mode == XIDirectTouch ? "direct" : "dependent");
					#endif
					break;
				}
			}
		}
	}
	XIFreeDeviceInfo(ds);
	return dev;
}

bool XWindow::handle(XIDeviceEvent &e)
{
	if (e.evtype != XI_ButtonPress && e.evtype != XI_ButtonRelease && e.evtype != XI_Motion)
	{
		assert(false); // didn't register for anything else
		return false;
	}
	Device &dev = devinfo(e.deviceid, display);
	DP("handle xi2 %d %d %g %g %d %d", (int)e.evtype, e.deviceid, e.event_x, e.event_y, e.flags, e.detail);

	if (!dev.valid){ assert(false); return false; }

	int buttons = e.detail;
	XIButtonState &bs = e.buttons;
	for (int i = 0; i < bs.mask_len; ++i)
	{
		unsigned char m = bs.mask[i];
		for (int j = 0; j < 8; ++j)
		{
			if (!(m & (1 << j))) continue;
			buttons |= Button1Mask << 8*i+j-1;
			DP(" -- button %d", 8*i+j);
		}
	}
	if (e.evtype != XI_Motion && !buttons) buttons = 1 << e.detail;

	XIModifierState &ms = e.mods;
	modifiers = ms.effective;
	DP(" -- mods %d", ms.effective);

	XIValuatorState &vs = e.valuators;
	bool scroll = false, move = false;
	double sx = 0.0, sy = 0.0, dx = 0.0, dy = 0.0;
	for (int i = 0, n = 0; i < vs.mask_len; ++i)
	{
		unsigned char m = vs.mask[i];
		for (int j = 0; j < 8; ++j)
		{
			if (!(m & (1 << j))) continue;
			double v = vs.values[n++];
			if (8*i+j < (int)dev.axis.size())
			{
				Device::Axis &a = dev.axis[8*i+j];
				if (!a.valid){ assert(false); continue; }
				double d = a.abs ? (isnan(a.value) ? 0.0 : v-a.value) : v;
				if (a.sx || a.sy)
				{
					if (a.unit > 0.0) d /= a.unit;
					(a.sx ? sx : sy) += d;
					dev.ignore_scrollbuttons = true;
					scroll = true;
					DP(" -- s[%c] = %g / %g", a.sx ? 'x' : 'y', a.sx ? sx : sy, a.unit);
				}
				else if (8*i+j < 2)
				{
					(j==0 ? dx : dy) += d;
					move = true;
					DP(" -- m[%d] = %g", j, d);
				}
				a.value = v;
			}
		}
	}

	if (scroll)
	{
		return handle_scroll(sx, sy, false);
	}
	switch (e.evtype)
	{
		case XI_ButtonPress:
		{
			return true;
		}
		case XI_ButtonRelease:
		{
			if (dev.ignore_scrollbuttons) return true;
			unsigned b = e.detail;
			int dx = 0, dy = 0;
			switch (b)
			{
				case ScrollUpButton:    --dy; break;
				case ScrollDownButton:  ++dy; break;
				case ScrollLeftButton:  --dx; break;
				case ScrollRightButton: ++dx; break;
			}
			if (!dx && !dy) return true;
			return handle_scroll(dx, dy, false);
		}
		case XI_Motion:
		{
			if (move && buttons)
			{
				DP(" -- motion %d", buttons);
				return handle_drag(buttons, dx, dy);
			}
			return true;
		}

	}
	return false;
}

bool XWindow::handle(XEvent &e)
{
	switch (e.type)
	{
		case ConfigureNotify:
			if (w != (unsigned)e.xconfigure.width ||
			    h != (unsigned)e.xconfigure.height)
			{
				w = e.xconfigure.width;
				h = e.xconfigure.height;
				reshape();
				return true;
			}
			break;

		case ClientMessage:
			if (e.xclient.message_type == XA_WM_PROTOCOLS)
			{
				if (e.xclient.data.l[0] == (int)XA_WM_DELETE_WINDOW)
				{
					closed = true;
					window = 0; // or not?
					return true;
				}
			}
			break;

		case DestroyNotify:
			closed = true;
			window = 0; // or not?
			return true;

		case GenericEvent:
			if (xi2 && e.xcookie.extension == xi2_opcode && XGetEventData(display, &e.xcookie))
			{
				bool ret = handle(*(XIDeviceEvent*)e.xcookie.data);
				XFreeEventData(display, &e.xcookie);
				return ret;
			}
			break;

		case KeyPress:
		case KeyRelease:
		{
			modifiers = e.xkey.state;
			KeySym key;
			char s[16];
			int len = XLookupString (&e.xkey, s, 16, &key, NULL);
			s[std::min(len, 15)] = 0;

			bool r = (e.type == KeyRelease);

			// ignore autorepeat keyreleases
			if (r && XEventsQueued(e.xkey.display, QueuedAfterReading))
			{
				XEvent nxt; XPeekEvent(e.xkey.display, &nxt);
				if (nxt.type == KeyPress && nxt.xkey.time == e.xkey.time && nxt.xkey.keycode == e.xkey.keycode)
				{
					return true;
				}
			}

			return handle_key(key, s, r);
		}

		case ButtonPress:
		{
			modifiers = e.xbutton.state;
			unsigned b = e.xbutton.button;
			if (b >= Button1 && b <= Button3)
			{
				mx = e.xbutton.x;
				my = e.xbutton.y;
				return true;
			}
			break;
		}
		case ButtonRelease:
		{
			modifiers = e.xbutton.state;
			unsigned b = e.xbutton.button;
			int dx = 0, dy = 0;
			switch (b)
			{
				case ScrollUpButton:    --dy; break;
				case ScrollDownButton:  ++dy; break;
				case ScrollLeftButton:  --dx; break;
				case ScrollRightButton: ++dx; break;
			}
			if (!dx && !dy) break;
			return handle_scroll(dx, dy, true);
		}

		case MotionNotify:
		{
			modifiers = e.xmotion.state;
			int dx = e.xmotion.x - mx, dy = e.xmotion.y - my;
			if (dx*dx + dy*dy < 4) return true;
			mx += dx; my += dy;
			int buttons = (modifiers & (Button1Mask|Button2Mask|Button3Mask));
			if (buttons) return handle_drag(buttons, dx, dy);
			break;
		}
	}
	return handle_other(e);
}
	

#include "stdafx.h"
#include "PlotView.h"
#include "SideView.h"
#include "MainWindow.h"
#include "CPlotApp.h"
#include "res/resource.h"
#include "Document.h"
#include "../Utility/System.h"

#include "gl/gl.h"
#include "gl/glu.h"

IMPLEMENT_DYNCREATE(PlotView, CWnd)
BEGIN_MESSAGE_MAP(PlotView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_CONTEXTMENU()

	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()

	ON_WM_KEYDOWN()
	ON_WM_SYSKEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SYSKEYUP()
END_MESSAGE_MAP()

PlotView::PlotView()
: dc(NULL), doc(NULL)
, tnf(-1.0)
, need_redraw(false)
, last_frame(-1.0)
, last_key(-1.0)
, rm(NULL)
, nums_on(0)
, mb(0)
{
	arrows.all = 0;
	memset(inertia, 0, 3 * sizeof(double));
	memset(nums, 0, 10 * sizeof(bool));
}

PlotView::~PlotView()
{
	delete rm;
	delete dc;
}

BOOL PlotView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_GROUP | WS_TABSTOP;
	return CWnd::PreCreateWindow(cs);
}

BOOL PlotView::Create(const RECT &rect, CWnd *parent, UINT ID)
{
	static bool init = false;
	if (!init)
	{
		WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hInstance = AfxGetInstanceHandle();
		wndcls.hCursor = theApp.LoadStandardCursor(IDC_ARROW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = _T("PlotView");
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(PlotView) failed");
		init = true;
	}

	return CWnd::Create(_T("PlotView"), NULL, WS_CHILD | WS_TABSTOP, rect, parent, ID);
}

int PlotView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) == -1) return -1;

	dc = new CClientDC(this);
	HDC hdc = dc->GetSafeHdc();

	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,                 // color depth
		0, 0, 0, 0, 0, 0,   // color bits ignored
		0,                  // no alpha buffer
		0,                  // shift bit ignored
		0,                  // no accumulation buffer
		0, 0, 0, 0,         // accum bits ignored
		32,                 // z-buffer depth
		0,                  // no stencil buffer
		0,                  // no auxiliary buffer
		PFD_MAIN_PLANE,     // main layer
		0,                  // reserved
		0, 0, 0             // layer masks ignored
	};
	int pf = ChoosePixelFormat(hdc, &pfd);
	if (!pf || !SetPixelFormat(hdc, pf, &pfd)) return -1;

	pf = ::GetPixelFormat(hdc);
	::DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);

	HGLRC ctx = wglCreateContext(hdc);
	wglMakeCurrent(hdc, ctx);

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		CStringA s; s.Format("GLEW init failed: %s", glewGetErrorString(err));
		AfxMessageBox(CString(s));
		return -1;
	}

	rm = new GL_RM(GL_Context(ctx));

	GetClientRect(&bounds);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	return 0;
}

void PlotView::OnDestroy()
{
	HGLRC ctx = ::wglGetCurrentContext();
	::wglMakeCurrent(NULL, NULL);
	if (ctx) ::wglDeleteContext(ctx);

	delete dc; dc = NULL;

	CWnd::OnDestroy();
}

//----------------------------------------------------------------------------------------------------------

BOOL PlotView::OnEraseBkgnd(CDC *dc)
{
	return TRUE;
}

void PlotView::OnPaint()
{
	CPaintDC pdc(this);
	if (!doc) return;
	Plot &plot = doc->plot;

	if (last_frame <= 0.0) reshape();
	last_frame = now();

	if (plot.axis_type() == Axis::Invalid)
	{
		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		double hr = plot.camera.aspect();
		glOrtho(-1.0, 1.0, -hr, hr, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW); glLoadIdentity();

		float y = 0.0f;
		glClearColor(y, y, y, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);

		double r = std::min(hr, 1.0 / hr)*0.5, x0 = 0.0, y0 = 0.0;
		y = 0.035f;
		glColor4f(y, y, y, 1.0f);
		auto tri = [&x0, &y0](double r, double a) { for (int i = 0; i<3; ++i, a += 2.0*M_PI / 3.0) glVertex2d(x0 + r*cos(a), y0 + r*sin(a)); };
		glBegin(GL_TRIANGLES);
		double a = last_frame*2.0*M_PI / 12.0;
		tri(r, a);
		a += M_PI;
		for (int i = 0; i < 3; ++i, a += 2.0*M_PI / 3.0)
		{
			x0 = r*cos(a); y0 = r*sin(a);
			tri(r*0.5, -0.75*a);
		}
		glEnd();

		glFinish();
		SwapBuffers(wglGetCurrentDC());
		need_redraw = false;
		//start();
		return;
	}

	bool dynamic = true; // todo: from pref
	bool anim = false;// dynamic && (!ikeys.empty() || modifiers & (Button1Mask | Button2Mask | Button3Mask));
	if (!anim && !plot.at_full_quality()) plot.update(CH_UNKNOWN);
	GL_CHECK;

	int nt = -1; // todo: pref
	if (nt < 1 || nt > 256) nt = n_cores;
	bool accum = false; // TODO
	plot.draw(*rm, nt, accum, anim);
	GL_CHECK;

	//status();
	//GL_CHECK;

	glFinish();
	SwapBuffers(wglGetCurrentDC());

	need_redraw = !plot.at_full_quality();
}

void PlotView::OnSize(UINT nType, int w, int h)
{
	CWnd::OnSize(nType, w, h);
	GetClientRect(&bounds);
	reshape();
}

void PlotView::reshape()
{
	if (!doc) return;
	Plot &plot = doc->plot;
	int w = bounds.Width(), h = bounds.Height();
	glViewport(0, 0, w, h);
	plot.camera.viewport(w, h, plot.axis);
	plot.update_axis();
	plot.update(CH_UNKNOWN);
}

//----------------------------------------------------------------------------------------------------------

COLORREF PlotView::GetBgColor()
{
	if (!doc || !doc->plot.number_of_graphs()) return GREY(0);
	return doc->plot.axis.options.background_color;
}

//----------------------------------------------------------------------------------------------------------
//  Mouse
//----------------------------------------------------------------------------------------------------------

void PlotView::OnButtonDown(int i, CPoint p)
{
	// activate
	::SendMessage(*GetParent(), WM_NEXTDLGCTL, (WPARAM)(HWND)*this, TRUE);

	m0 = p;
	if (!mb)
	{
		SetCapture();
		// StartAnimation();
	}
	mb |= 1 << i;
}
void PlotView::OnLButtonDown(UINT flags, CPoint p) { OnButtonDown(0, p); }
void PlotView::OnMButtonDown(UINT flags, CPoint p) { OnButtonDown(1, p); }
void PlotView::OnRButtonDown(UINT flags, CPoint p) { OnButtonDown(2, p); }

void PlotView::OnButtonUp(int i, CPoint p)
{
	mb &= ~(1 << i);
	if (!mb)
	{
		ReleaseCapture();
		// EndAnimation();
	}
}
void PlotView::OnLButtonUp(UINT flags, CPoint p) { OnButtonUp(0, p); }
void PlotView::OnMButtonUp(UINT flags, CPoint p) { OnButtonUp(1, p); }
void PlotView::OnRButtonUp(UINT flags, CPoint p) { OnButtonUp(2, p); }

/*static void ClearQueue(NSEvent *e, NSUInteger mask, std::function<void(NSEvent*)> handler)
{
	while (e)
	{
		handler(e);
		e = [NSApp nextEventMatchingMask : mask untilDate : nil inMode : NSDefaultRunLoopMode dequeue : YES];
	}
}*/

enum
{
	CONTROL = 1,
	ALT     = 2,
	SHIFT   = 4,
	WIN     = 8
};

void PlotView::OnMouseMove(UINT flags, CPoint p)
{
	if (!mb) return;
	//clearQueue(event, NSOtherMouseDraggedMask, [&dx, &dy, &dz](NSEvent *e) { dx += e.deltaX; dy += e.deltaY; dz += e.deltaZ; });
	int mods = (flags & MK_CONTROL) * CONTROL | (flags & MK_SHIFT) * SHIFT;
	CPoint d = p - m0; m0 = p;
	if (mb & 1) // left
	{
		move(d.x, d.y, mods);
	}
	if (mb & 2) // middle
	{
		move(d.x, d.y, mods | ALT);
	}
	if (mb & 4) // right
	{
		move(d.x, d.y, mods | WIN);
	}

	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
	sv.UpdateAxis();
	Invalidate();
}

BOOL PlotView::OnMouseWheel(UINT flags, short dz, CPoint p)
{
	if (mb) return TRUE; // probably accidental
	//clearQueue(event, NSScrollWheelMask, [&dx, &dy, &dz](NSEvent *e)
	int mods = (flags & MK_CONTROL) * CONTROL | (flags & MK_SHIFT) * SHIFT;
	zoom(-0.03*dz, mods);

	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
	sv.UpdateAxis();
	Invalidate();
	return TRUE;
}

//----------------------------------------------------------------------------------------------------------

void PlotView::zoom(double dy, int flags)
{
	if (!doc || fabs(dy) < 1e-5) return;
	Plot &plot = doc->plot;
	Axis &axis = plot.axis;
	bool shift = flags & SHIFT;
	bool   alt = flags & ALT;
	double   f = exp(-dy * 0.02);

	switch (plot.axis.type())
	{
		case Axis::Rect:
			if (alt)
			{
				plot.axis.in_zoom(1.0 / f);
				plot.update(CH_IN_RANGE);
			}
			else
			{
				CPoint p; GetCursorPos(&p); ScreenToClient(&p);
				double x0 = 2.0 * (p.x - bounds.left) / bounds.Width() - 1.0; // [-1,1]
				double y0 = 2.0 * (p.y - bounds.top) / bounds.Height() - 1.0;
				double x1 = (x0 * axis.range(0)) + axis.center(0);
				double y1 = (y0 * axis.range(1)) + axis.center(1);
				axis.zoom(f);
				if (fabs(x0) < 0.9 && fabs(y0) < 0.9 * bounds.Height() / bounds.Width())
				{
					double x2 = (x0 * axis.range(0)) + axis.center(0);
					double y2 = (y0 * axis.range(1)) + axis.center(1);
					axis.move(x1 - x2, y1 - y2, 0.0);
				}
				plot.update(CH_AXIS_RANGE);
			}
			break;

		default:
			if (alt)
			{
				axis.in_zoom(1.0 / f);
				plot.update(CH_IN_RANGE);
			}
			else if (shift)
			{
				axis.zoom(f);
				plot.update(CH_AXIS_RANGE);
			}
			else
			{
				plot.camera.zoom(f);
			}
			break;
	}
}

static inline double absmax(double a, double b) { return fabs(a) > fabs(b) ? a : b; }

void PlotView::move(double dx, double dy, int flags)
{
	if (fabs(dx) < 1e-5 && fabs(dy) < 1e-5) return;
	if (!doc) return;

	Plot   &plot = doc->plot;
	Axis   &axis = plot.axis;
	Camera &camera = plot.camera;
	bool   shift = flags & SHIFT;
	bool     alt = flags & ALT;
	bool     cmd = flags & WIN;
	double pixel = plot.pixel_size();

	switch (axis.type())
	{
		case Axis::Rect:
			if (alt && shift)
			{
				axis.in_zoom(exp(absmax(dx, dy) * 0.01));
				plot.update(CH_IN_RANGE);
			}
			else if (alt)
			{
				dx *= pixel;
				dy *= pixel;
				axis.in_move(dx, -dy);
				plot.update(CH_IN_RANGE);
			}
			else if (shift)
			{
				zoom(0.5*absmax(dx, dy), 0);
			}
			else
			{
				dx *= pixel;
				dy *= pixel;
				axis.move(-dx, dy, 0.0);
				plot.update(CH_AXIS_RANGE);
			}
			break;

		default:
			if (cmd)
			{
				float f;
				camera.scalefactor(0, f);
				pixel /= f;
				camera.move(axis, -dx*pixel, dy*pixel, 0);
				plot.update(CH_AXIS_RANGE);
			}
			else if (alt && shift)
			{
				axis.in_zoom(exp(absmax(dx, dy) * 0.01));
				plot.update(CH_IN_RANGE);
			}
			else if (alt)
			{
				axis.in_move(dx*0.01, -dy*0.01);
				plot.update(CH_IN_RANGE);
			}
			else if (shift)
			{
				if (fabs(dy) > fabs(dx))
				{
					camera.zoom(exp(-dy * 0.01));
				}
				else
				{
					axis.zoom(exp(-dx * 0.01));
					plot.update(CH_AXIS_RANGE);
				}
			}
			else
			{
				camera.rotate(0.01 * dy, 0, 0.01 * dx);
			}
			break;
	}
}

//----------------------------------------------------------------------------------------------------------------------
//  keyboard
//----------------------------------------------------------------------------------------------------------------------

static inline void accel(double &inertia, double dt)
{
	inertia += 0.4*std::max(1.0, dt) + inertia*0.4;
	inertia = std::min(inertia, 20.0);
}

void PlotView::handleArrows()
{
	//assert(animating());

	int dx = 0, dy = 0, dz = 0;
	if (arrows.left)  --dx;
	if (arrows.right) ++dx;
	if (arrows.up)    --dy;
	if (arrows.down)  ++dy;
	if (arrows.plus)  ++dz;
	if (arrows.minus) --dz;

	if (nums_on)
	{
		for (int i = 0; i < 10; ++i)
		{
			//if (nums[i])[self.document.parameterBox changeParameter : i by : cnum(dx, -dy)];
		}
	}
	else
	{
		double t0 = now();
		double dt = t0 - last_key;
		double dt0 = 1.0 / 60.0;
		last_key = t0;
		double scale = (dt0 > 0 ? std::min(dt / dt0, 5.0) : 1.0);

		if (dx) accel(inertia[0], scale);
		if (dy) accel(inertia[1], scale);
		if (dz) accel(inertia[2], scale);

		//NSLog(@"arrow timer %g", scale);

		int flags = 0;// [NSEvent modifierFlags];
		if (arrows.plus || arrows.minus) flags |= SHIFT;

		move(dx*inertia[0], dy*inertia[1] + dz*inertia[2], flags);
	}
}

void PlotView::OnKeyUp(UINT c, UINT rep, UINT flags)
{
	switch (c)
	{
		case  VK_LEFT: arrows.left  = false; inertia[0] = 0.0; break;
		case VK_RIGHT: arrows.right = false; inertia[0] = 0.0; break;
		case    VK_UP: arrows.up    = false; inertia[1] = 0.0; break;
		case  VK_DOWN: arrows.down  = false; inertia[1] = 0.0; break;
		case      '+': arrows.plus  = false; inertia[2] = 0.0; break;
		case      '-': arrows.minus = false; inertia[2] = 0.0; break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			int i = (c - '0' + 9) % 10;
			if (nums[i]) { --nums_on; nums[i] = false; break; }
			break;
		}
	}
	if (!arrows.all)
	{
		//STOP_TIMER(arrowKeyTimer);
		//[self endAnimation];

		SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
		sv.UpdateAxis();
	}
}

void PlotView::OnKeyDown(UINT c, UINT rep, UINT flags)
{
	if (flags & KF_REPEAT) return;
	if (!doc) return;

	const bool ctrl  = GetKeyState(VK_CONTROL) & 0x8000;
	const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
	const bool alt   = c & (1 << 13);
	const bool win   = GetKeyState(VK_RMENU) & 0x8000;

	bool done = true;
	switch (c)
	{
		case  VK_LEFT: arrows.left  = true; break;
		case VK_RIGHT: arrows.right = true; break;
		case    VK_UP: arrows.up    = true; break;
		case  VK_DOWN: arrows.down  = true; break;
		case      '+': arrows.plus  = true; break;
		case      '-': arrows.minus = true; break;
		default: done = false;
	}

	if (done)
	{
		/*if (arrows.all && !arrowKeyTimer)
		{
			TIMER(arrowKeyTimer, handleArrows, 1.0 / 60.0);
			[self startAnimation];
		}*/
		return;
	}

	Plot &plot = doc->plot;
	Axis &axis = plot.axis;

	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();

	if (c >= 'A' && c <= 'Z' && !shift) c += 'a' - 'A';

	switch (c)
	{
		case 'a': sv.OnDrawAxis(); break;
		case 'd': sv.OnDisco(); break;

		case 'c': sv.OnClip(); break;
		case 'C': sv.OnClipCustom(); break;
		case 'l': sv.OnClipLock(); break;
		case 'L': sv.OnClipReset(); break;

		case 'u': sv.OnTopView(); break;
		case 'U': sv.OnBottomView(); break;
		case 'f': sv.OnFrontView(); break;
		case 'F': sv.OnBackView(); break;
		case 'r': sv.OnRightView(); break;
		case 'R': sv.OnLeftView(); break;
		case 'z': sv.OnCenterAxis(); break;
		case 'e': sv.OnEqualRanges(); break;

		/*case 'g': [settingsBox toggle : settingsBox.gridMode];   return;

		case 't': [settingsBox  cycle : settingsBox.textureMode direction : +1]; return;
		case 'T': [settingsBox  cycle : settingsBox.textureMode direction : -1]; return;
		case 'v': [settingsBox  cycle : settingsBox.vfMode      direction : +1]; return;
		case 'V': [settingsBox  cycle : settingsBox.vfMode      direction : -1]; return;


		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			int i = (c - '0' + 9) % 10;
			if (i < self.document.parameterBox.numberOfParameters)
			{
				if (!nums[i]) ++nums_on;
				nums[i] = true;
				return;
			}
			// fallthrough
		}*/
	}
}

void PlotView::OnSysKeyDown(UINT c, UINT rep, UINT flags)
{}
void PlotView::OnSysKeyUp(UINT c, UINT rep, UINT flags)
{}

#include "stdafx.h"
#include "PlotView.h"
#include "SideView.h"
#include "MainWindow.h"
#include "CPlotApp.h"
#include "res/resource.h"
#include "Document.h"
#include "../Utility/System.h"
#include "MainView.h"

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
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

PlotView::PlotView()
: dc(NULL)
, tnf(-1.0)
, last_frame(-1.0)
, last_key(-1.0)
, rm(NULL)
, nums_on(0)
, timer(1.0/60.0)
, drop(DropHandler::CPLOT, [this](const CString &f) { return load(f); })
, in_resize(false)
{
	mb.all = 0;
	arrows.all = 0;
	memset(inertia, 0, 3 * sizeof(double));
	memset(nums, 0, 10 * sizeof(bool));
	timer.callback = [this]() { Invalidate(); };
}

PlotView::~PlotView()
{
	delete rm;
	delete dc;
}

Document &PlotView::document() const
{
	MainWindow *w = (MainWindow*)GetParentFrame();
	assert(w);
	assert(w->doc);
	return *w->doc;
}

UINT PlotView::OnGetDlgCode()
{
	return CWnd::OnGetDlgCode() | DLGC_WANTARROWS | DLGC_WANTCHARS;
}

BOOL PlotView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CHILD | WS_GROUP | WS_TABSTOP;
	return CWnd::PreCreateWindow(cs);
}

BOOL PlotView::Create(const RECT &rect, CWnd *parent, UINT ID)
{
	static bool init = false;
	if (!init)
	{
		WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
		wndcls.style = CS_DBLCLKS;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hInstance = AfxGetInstanceHandle();
		wndcls.hCursor = theApp.LoadStandardCursor(IDC_ARROW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = _T("PlotView");
		wndcls.hbrBackground = NULL;
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

	drop.Register(this);

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
	if (!in_resize) return false;
	CRect bounds; GetClientRect(bounds);
	CBrush bg(GetBgColor());
	dc->FillRect(bounds, &bg);
	in_resize = false;
	return TRUE;
}

bool PlotView::animating()
{
	bool anim = (document().plot.axis_type() == Axis::Invalid || arrows.all || 
		((MainWindow*)GetParentFrame())->GetSideView().Animating());

	if (!anim && mb.all)
	{
		// mouse drags should trigger animation state in color graphs
		Plot &plot = document().plot;
		for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
		{
			const Graph *g = plot.graph(i);
			if (g->options.hidden) continue;
			if (g->isColor())
			{
				anim = true;
				break;
			}
		}
	}

	if (anim)
	{
		if (!timer.running()) timer.start();
	}
	else
	{
		if (timer.running()) timer.stop();
	}
	return anim;
}

void PlotView::OnPaint()
{
	CPaintDC pdc(this);
	Plot &plot = document().plot;

	if (last_frame <= 0.0) reshape();
	last_frame = now();

	bool anim = animating();

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
		return;
	}

	if (anim)
	{
		handleArrows();
		((MainWindow*)GetParentFrame())->GetSideView().Animate();
	}

	if (!anim && !plot.at_full_quality()) plot.update(CH_UNKNOWN);
	GL_CHECK;

	int nt = Preferences::threads();
	if (nt < 1 || nt > 256) nt = n_cores;
	bool accum = false; // TODO
	bool dynamic = Preferences::dynamic();
	plot.draw(*rm, nt, accum, anim && dynamic);
	GL_CHECK;

	//status();
	//GL_CHECK;

	glFinish();
	SwapBuffers(wglGetCurrentDC());
}

void PlotView::OnSize(UINT nType, int w, int h)
{
	CWnd::OnSize(nType, w, h);
	GetClientRect(&bounds);
	reshape();
	in_resize = true;
}

void PlotView::reshape()
{
	Plot &plot = document().plot;
	int w = bounds.Width(), h = bounds.Height();
	glViewport(0, 0, w, h);
	plot.camera.viewport(w, h, plot.axis);
	plot.update_axis();
	plot.update(CH_UNKNOWN);
}

//----------------------------------------------------------------------------------------------------------

COLORREF PlotView::GetBgColor()
{
	if (!document().plot.number_of_graphs()) return GREY(0);
	return document().plot.axis.options.background_color;
}

//----------------------------------------------------------------------------------------------------------
//  Mouse
//----------------------------------------------------------------------------------------------------------

void PlotView::OnButtonDown(int i, CPoint p)
{
	// activate
	((MainWindow*)GetParentFrame())->Focus(this);

	m0 = p;
	if (!mb.all)
	{
		SetCapture();
		// StartAnimation();
	}
	mb.all |= 1 << i;
}
void PlotView::OnLButtonDown(UINT flags, CPoint p) { OnButtonDown(0, p); }
void PlotView::OnMButtonDown(UINT flags, CPoint p) { OnButtonDown(1, p); }
void PlotView::OnRButtonDown(UINT flags, CPoint p) { OnButtonDown(2, p); }

void PlotView::OnButtonUp(int i, CPoint p)
{
	mb.all &= ~(1 << i);
	if (!mb.all)
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
	SHIFT   = 4
};

void PlotView::OnMouseMove(UINT flags, CPoint p)
{
	if (!mb.all) return;
	//clearQueue(event, NSOtherMouseDraggedMask, [&dx, &dy, &dz](NSEvent *e) { dx += e.deltaX; dy += e.deltaY; dz += e.deltaZ; });
	int mods = ((flags & MK_CONTROL) ? CONTROL : 0) | ((flags & MK_SHIFT) ? SHIFT : 0);
	CPoint d = p - m0; m0 = p;
	if (mb.left)
	{
		move(d.x, d.y, mods);
	}
	if (mb.middle)
	{
		move(d.x, d.y, mods | ALT);
	}
	if (mb.right)
	{
		move(d.x, d.y, mods | CONTROL);
	}

	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
	sv.UpdateAxis(false);
	Invalidate();
}

BOOL PlotView::OnMouseWheel(UINT flags, short dz, CPoint p)
{
	if (mb.middle) return TRUE; // probably accidental
	//clearQueue(event, NSScrollWheelMask, [&dx, &dy, &dz](NSEvent *e)

	if (mb.right)
	{
		zoom(-0.03*dz, ALT);
	}
	else
	{
		int mods = ((flags & MK_CONTROL) ? CONTROL : 0) | ((flags & MK_SHIFT) ? SHIFT : 0);
		zoom(-0.03*dz, mods);
	}

	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
	sv.UpdateAxis(false);
	Invalidate();
	return TRUE;
}

//----------------------------------------------------------------------------------------------------------

void PlotView::zoom(double dy, int flags)
{
	if (fabs(dy) < 1e-5) return;
	Plot &plot = document().plot;
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

	Plot   &plot = document().plot;
	Axis   &axis = plot.axis;
	Camera &camera = plot.camera;
	bool   shift = flags & SHIFT;
	bool     alt = flags & ALT;
	bool     cmd = flags & CONTROL;
	double pixel = plot.pixel_size();

	switch (axis.type())
	{
		case Axis::Rect:
			if (cmd && shift)
			{
				axis.in_zoom(exp(absmax(dx, -dy) * 0.01));
				plot.update(CH_IN_RANGE);
			}
			else if (cmd)
			{
				dx *= pixel;
				dy *= pixel;
				axis.in_move(dx, -dy);
				plot.update(CH_IN_RANGE);
			}
			else if (shift)
			{
				zoom(0.5*absmax(dx, -dy), 0);
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
			if (alt)
			{
				pixel *= camera.scalefactor();
				camera.move(axis, -dx*pixel, dy*pixel, 0);
				plot.update(CH_AXIS_RANGE);
			}
			else if (cmd && shift)
			{
				axis.in_zoom(exp(absmax(dx, dy) * 0.01));
				plot.update(CH_IN_RANGE);
			}
			else if (cmd)
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
	int dx = 0, dy = 0, dz = 0;
	if (arrows.left)  --dx;
	if (arrows.right) ++dx;
	if (arrows.up)    --dy;
	if (arrows.down)  ++dy;
	if (arrows.plus)  ++dz;
	if (arrows.minus) --dz;

	double t0 = now();
	double dt = t0 - last_key;
	double dt0 = 1.0 / 60.0;
	last_key = t0;
	double scale = (dt0 > 0 ? std::min(dt / dt0, 5.0) : 1.0);

	if (dx) accel(inertia[0], scale);
	if (dy) accel(inertia[1], scale);
	if (dz) accel(inertia[2], scale);

	if (nums_on)
	{
		SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
		for (int i = 0; i < 10; ++i)
		{
			if (!nums[i]) continue;
			sv.params.Change(i, cnum(dx*inertia[0], -dy*inertia[1]) * 0.005);
		}
	}
	else
	{
		int flags = 0;// [NSEvent modifierFlags];

		if (GetKeyState(VK_SHIFT) & 0x8000) flags |= SHIFT;
		//const bool alt = c & (1 << 13);
		//if (GetKeyState(VK_RMENU) & 0x8000) flags |= WIN;
		if (GetKeyState(VK_CONTROL) & 0x8000) flags |= CONTROL;

		if (dx || dy) move(dx*inertia[0], dy*inertia[1], flags);
		if (dz) move(0, dz*inertia[2], flags | SHIFT);
	}
}

void PlotView::OnKeyUp(UINT c, UINT rep, UINT flags)
{
	switch (c)
	{
		case      VK_LEFT: arrows.left  = false; inertia[0] = 0.0; break;
		case     VK_RIGHT: arrows.right = false; inertia[0] = 0.0; break;
		case        VK_UP: arrows.up    = false; inertia[1] = 0.0; break;
		case      VK_DOWN: arrows.down  = false; inertia[1] = 0.0; break;
		case  VK_OEM_PLUS:
		case       VK_ADD: arrows.plus  = false; inertia[2] = 0.0; break;
		case VK_OEM_MINUS:
		case  VK_SUBTRACT: arrows.minus = false; inertia[2] = 0.0; break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			int i = (c - '0' + 9) % 10;
			if (nums[i]) { --nums_on; nums[i] = false; }
			break;
		}
	}
	if (!arrows.all)
	{
		animating();

		SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();
		sv.UpdateAxis(false);
	}
}

void PlotView::OnKeyDown(UINT c, UINT rep, UINT flags)
{
	if (flags & KF_REPEAT) return;

	const bool ctrl  = GetKeyState(VK_CONTROL) & 0x8000;
	const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
	const bool alt   = c & (1 << 13);
	const bool win   = GetKeyState(VK_RMENU) & 0x8000;

	bool done = true;
	switch (c)
	{
		case      VK_LEFT: arrows.left  = true; break;
		case     VK_RIGHT: arrows.right = true; break;
		case        VK_UP: arrows.up    = true; break;
		case      VK_DOWN: arrows.down  = true; break;
		case  VK_OEM_PLUS: 
		case       VK_ADD: arrows.plus  = true; break;
		case VK_OEM_MINUS:
		case  VK_SUBTRACT: arrows.minus = true; break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			int i = (c - '0' + 9) % 10;
			if (!nums[i]) { ++nums_on; nums[i] = true; }
			break;
		}

		default: done = false;
	}

	if (done)
	{
		if (arrows.all) animating();
		return;
	}

	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();

	if (c >= 'A' && c <= 'Z' && !shift) c += 'a' - 'A';

	switch (c)
	{
		case 'a': sv.settings.OnDrawAxis(); break;
		case 'd': sv.settings.OnDisco(); break;

		case 'c': sv.settings.OnClip(); break;
		case 'C': sv.settings.OnClipCustom(); break;
		case 'l': sv.settings.OnClipLock(); break;
		case 'L': sv.settings.OnClipReset(); break;
		case 'g': sv.settings.OnToggleGrid(); break;

		case 't': sv.style.OnCycleTextureMode(+1); return;
		case 'T': sv.style.OnCycleTextureMode(-1); return;
		case 'v': sv.settings.OnCycleVFMode(+1); return;
		case 'V': sv.settings.OnCycleVFMode(-1); return;

		case 'u': sv.axis.OnTopView(); break;
		case 'U': sv.axis.OnBottomView(); break;
		case 'f': sv.axis.OnFrontView(); break;
		case 'F': sv.axis.OnBackView(); break;
		case 'r': sv.axis.OnRightView(); break;
		case 'R': sv.axis.OnLeftView(); break;
		case 'z': sv.axis.OnCenterAxis(); break;
		case 'e': sv.axis.OnEqualRanges(); break;

		case VK_OEM_PERIOD:
			sv.params.StopAllAnimation();
			break;

		case ' ':
		{
			if (nums_on)
			{
				for (int i = 0; i < 10; ++i)
				{
					if (nums[i]) sv.params.ToggleAnimation(i);
				}
			}
			else
			{
				sv.params.ToggleAnimation(0);
			}
			break;
		}
	}
}

void PlotView::OnChar(UINT c, UINT rep, UINT flags)
{
	// don't care, just don't beep on every key
}

bool PlotView::load(const CString &f)
{
	MainWindow* w = (MainWindow*)AfxGetMainWnd();
	w->OnFocusGraph();

	auto &d = document();
	if (!d.OnOpenDocument(f)) return false;
	d.SetPathName(f);
	d.UpdateAllViews(NULL);

	w->GetSideView().UpdateAll();
	w->GetMainView().Update();
	Invalidate();
	return true;
}

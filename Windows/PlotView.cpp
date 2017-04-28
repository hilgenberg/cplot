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
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

PlotView::PlotView()
: dc(NULL), doc(NULL)
, tnf(-1.0)
, last_frame(-1.0)
, rm(NULL)
{
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
	if (!doc || !doc->plot.number_of_graphs()) return RGB(0, 0, 0);
	return doc->plot.axis.options.background_color;
}

void PlotView::OnLButtonDown(UINT flags, CPoint p)
{
	::SendMessage(*GetParent(), WM_NEXTDLGCTL, (WPARAM)(HWND)*this, TRUE);
	CWnd::OnLButtonDown(flags, p);
}

void PlotView::OnKeyDown(UINT c, UINT rep, UINT flags)
{
	SideView &sv = ((MainWindow*)GetParentFrame())->GetSideView();

	switch (c)
	{
		case 'D': sv.OnDisco(); break;
		case 'A': sv.OnDrawAxis(); break;
	}
}

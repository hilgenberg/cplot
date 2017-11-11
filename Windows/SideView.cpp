#include "stdafx.h"
#include "CPlotApp.h"
#include "SideView.h"
#include "Document.h"
#include "Controls/SplitterWnd.h"
#include "MainWindow.h"
#include "MainView.h"
#include "Controls/ViewUtil.h"
#include "res/resource.h"

enum
{
	ID_params = 1050,
	ID_defs,
	ID_graphs,
	ID_settings,
	ID_axis
};

IMPLEMENT_DYNCREATE(SideView, CFormView)
BEGIN_MESSAGE_MAP(SideView, CFormView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEHWHEEL()
END_MESSAGE_MAP()

SideView::SideView()
: CFormView(IDD_SIDEVIEW)
, active_anims(0)
{
}

void SideView::Redraw()
{
	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().GetPlotView().Invalidate();
}
void SideView::Recalc(Plot &plot)
{
	plot.update(-1);
	Redraw();
}
void SideView::Recalc(Graph *g)
{
	g->update(-1);
	Redraw();
}

Document &SideView::document() const
{
	return *((MainWindow*)GetParentFrame())->doc;
}

BOOL SideView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFormView::PreCreateWindow(cs)) return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.style &= ~WS_HSCROLL;
	//cs.style |= WS_TABSTOP| WS_GROUP;
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL);
	
	return TRUE;
}

void SideView::OnInitialUpdate()
{
	UpdateAll();
	CFormView::OnInitialUpdate();
}

BOOL SideView::OnEraseBkgnd(CDC *dc)
{
	RECT bounds; GetClientRect(&bounds);
	CBrush bg(GetSysColor(COLOR_BTNFACE));
	dc->FillRect(&bounds, &bg);
	return TRUE;
}

void SideView::OnSize(UINT type, int w, int h)
{
	CFormView::OnSize(type, w, h);
	UpdateAll();
}

BOOL SideView::OnMouseWheel(UINT flags, short dz, CPoint p)
{
	UINT n; ::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &n, 0);
	if (n == WHEEL_PAGESCROLL)
	{
		SendMessage(WM_VSCROLL, MAKEWPARAM(dz > 0 ? SB_PAGEUP : SB_PAGEDOWN, 0), 0);
	}
	else
	{
		for (int i = 0, k = abs(dz)*n / WHEEL_DELTA; i < k; ++i)
			SendMessage(WM_VSCROLL, MAKEWPARAM(dz > 0 ? SB_LINEUP : SB_LINEDOWN, 0), 0);
	}
	return TRUE;
}

//---------------------------------------------------------------------------------------------
// Headers
//---------------------------------------------------------------------------------------------

BoxState SideView::GetBoxState() const
{
	BoxState b; b.all = 0;
	b.params   = params.header.GetCheck();
	b.defs     = defs.header.GetCheck();
	b.graphs   = graphs.header.GetCheck();
	b.settings = settings.header.GetCheck();
	b.axis     = axis.header.GetCheck();
	return b;
}

void SideView::SetBoxState(BoxState b)
{
	params.header.SetCheck(b.params);
	defs.header.SetCheck(b.defs);
	graphs.header.SetCheck(b.graphs);
	settings.header.SetCheck(b.settings);
	axis.header.SetCheck(b.axis);
}

//---------------------------------------------------------------------------------------------
// Create & Update
//---------------------------------------------------------------------------------------------

int SideView::OnCreate(LPCREATESTRUCT cs)
{
	if (CFormView::OnCreate(cs) < 0) return -1;

	EnableScrollBarCtrl(SB_HORZ, FALSE);

	params.Create(this, ID_params);
	defs.Create(this, ID_defs);
	graphs.Create(this, ID_graphs);
	settings.Create(this, ID_settings);
	axis.Create(this, ID_axis);

	return 0;
}

void SideView::UpdateAll()
{
	CRect bounds; GetClientRect(bounds);
	if (bounds.Width() < 2) return;
	const int W = bounds.Width();

	params.Width(W);
	params.Update(true);

	defs.Width(W);
	defs.Update(true);

	graphs.Width(W);
	graphs.Update(true);

	settings.Width(W);
	settings.Update(true);

	axis.Width(W);
	axis.Update(true);

	Update(false);
}

void SideView::Update(bool full)
{
	if (full) { UpdateAll(); return; }

	CRect bounds; GetClientRect(bounds);
	if (bounds.Width() < 2) return;

	const int W = bounds.Width();
	const int y0 = -GetScrollPos(SB_VERT);
	int y = y0;

	params.Position(y);
	defs.Position(y);
	graphs.Position(y);
	settings.Position(y);
	axis.Position(y);

	DS0;
	const int h_row = DS(24);
	EnableScrollBarCtrl(SB_HORZ, FALSE);
	SetScrollSizes(MM_TEXT, CSize(W, std::max(y - y0, bounds.Height())), CSize(W, bounds.Height()), CSize(h_row, h_row));

	GetParent()->Invalidate();
}

//---------------------------------------------------------------------------------------------
// Animation
//---------------------------------------------------------------------------------------------

void SideView::AnimStateChanged(bool active)
{
	active_anims += active ? 1 : -1;
	assert(active_anims >= 0);
	if (active_anims == 1) Redraw();
}

bool SideView::Animating() const
{
	return active_anims;
}

void SideView::Animate()
{
	if (!active_anims) return;
	double t = now();
	params.Animate(t);
	axis.Animate(t);
}

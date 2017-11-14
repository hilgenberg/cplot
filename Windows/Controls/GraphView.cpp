#include "../stdafx.h"
#include "GraphView.h"
#include "../Document.h"
#include "../res/resource.h"
#include "ViewUtil.h"
#include "../SideView.h"
#include "../MainWindow.h"
#include "../MainView.h"
#include "../CPlotApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum
{
	ID_desc = 2000,
	ID_visible
};

IMPLEMENT_DYNAMIC(GraphView, CWnd)
BEGIN_MESSAGE_MAP(GraphView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_visible, OnVisible)
	ON_BN_CLICKED(ID_desc,    OnSelect)
	//ON_LBN_DBLCLK(ID_name, OnEdit)
END_MESSAGE_MAP()

GraphView::GraphView(SideSectionGraphs &parent, Graph &g)
: parent(parent), g_id(g.oid())
{
}

BOOL GraphView::PreCreateWindow(CREATESTRUCT &cs)
{
	cs.style |= WS_CHILD;
	cs.dwExStyle |= WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT;
	return CWnd::PreCreateWindow(cs);
}

BOOL GraphView::Create(const RECT &rect, CWnd *parent, UINT ID)
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
		wndcls.lpszClassName = _T("GraphView");
		wndcls.hbrBackground = NULL;
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(GraphView) failed");
		init = true;
	}

	return CWnd::Create(_T("GraphView"), NULL, WS_CHILD | WS_TABSTOP, rect, parent, ID);
}

int GraphView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) < 0) return -1;
	EnableScrollBarCtrl(SB_BOTH, FALSE);

	START_CREATE;

	desc.Create(_T(""), WS_CHILD | BS_PUSHBUTTON, whatever, this, ID_desc);
	desc.SetFont(&controlFont());
	desc.m_nFlatStyle = CMFCButton::BUTTONSTYLE_NOBORDERS;
	desc.m_bDrawFocus = false;
	desc.m_nAlignStyle = CMFCButton::ALIGN_LEFT;
	
	CHECK(visible, "");
	return 0;
}

int GraphView::height(int w) const
{
	DS0;
	return DS(22);
}

void GraphView::OnVisible()
{
	Graph *g = graph(); if (!g) return;
	g->options.hidden = !g->options.hidden;
	visible.SetCheck(!g->options.hidden);
	Plot &plot = parent.document().plot;
	plot.update_axis();
	parent.Recalc(g);
}

void GraphView::OnSelect()
{
	Graph *g = graph(); if (!g) return;
	Plot &plot = parent.document().plot;
	plot.set_current_graph(g);

	parent.parent().UpdateAll();
	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().Update();
}

void GraphView::Update(bool full)
{
	CRect bounds; GetClientRect(bounds);
	Graph *g = graph();
	if (!g) return;
	DS0;
	
	visible.SetCheck(!g->options.hidden);
	desc.SetWindowText(Convert(g->description_line()));
	Plot &plot = parent.document().plot;
	bool bold = (plot.number_of_graphs() > 1 && plot.current_graph() == g);
	desc.SetFont(&controlFont(bold));

	if (!full) return;

	const int W = bounds.Width();
	const int SPC = DS(5); // amount of spacing
	const int w1 = DS(20); // checkbox width
	const int x0 = SPC;  // row x start / amount of space on the left
	const int x1 = W - SPC;
	int y = 0; // y for next control

	const int h_label = DS(14), h_edit = DS(20), h_delta = DS(20), h_button = DS(20), h_row = DS(22);

	MOVE(visible, x0, x0 + w1, y, h_button, h_row);
	MOVE(desc, x0 + w1 + SPC, x1, y, h_label, h_row);
}

void GraphView::OnInitialUpdate()
{
	Update(true);
}

void GraphView::OnSize(UINT type, int w, int h)
{
	CWnd::OnSize(type, w, h);
	EnableScrollBarCtrl(SB_BOTH, FALSE);
	Update(true);
}

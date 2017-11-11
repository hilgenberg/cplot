#include "../stdafx.h"
#include "SideSection.h"
#include "ViewUtil.h"
#include "../res/resource.h"
#include "../SideView.h"
#include "../MainWindow.h"
#include "../../Graphs/Plot.h"
#include "../../Graphs/Graph.h"
#include "../PlotView.h"
#include "../Document.h"
#include "../MainView.h"
#include "../CPlotApp.h"
#include "../SideView.h"

enum
{
	ID_header = 1050
};

BEGIN_MESSAGE_MAP(SideSection, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(ID_header, OnHeader)
END_MESSAGE_MAP()

SideSection::SideSection()
{
}

Document &SideSection::document() const { return parent().document(); }
void SideSection::Redraw()
{
	window()->GetMainView().GetPlotView().Invalidate();
}
void SideSection::Recalc(Plot &plot)
{
	plot.update(-1);
	Redraw();
}
void SideSection::Recalc(Graph *g)
{
	g->update(-1);
	Redraw();
}
Plot &SideSection::GetPlot() const { return document().plot; }
Graph *SideSection::GetGraph() const { return document().plot.current_graph(); }

BOOL SideSection::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs)) return FALSE;

	//cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.style &= ~WS_HSCROLL;
	//cs.style |= WS_TABSTOP| WS_GROUP;
	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS,
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL);
	
	return TRUE;
}

BOOL SideSection::OnEraseBkgnd(CDC *dc)
{
	RECT bounds; GetClientRect(&bounds);
	CBrush bg(GetSysColor(COLOR_BTNFACE));
	dc->FillRect(&bounds, &bg);
	return TRUE;
}

//---------------------------------------------------------------------------------------------
// Headers
//---------------------------------------------------------------------------------------------

void SideSection::OnHeader()
{
	Update(true);
	parent().Update(false);
}

BOOL SideSection::Create(SideView *parent, UINT ID)
{
	static std::set<CString> reg;
	CString cls = wndClassName();
	if (!reg.count(cls))
	{
		WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
		wndcls.style = CS_DBLCLKS;// | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hInstance = AfxGetInstanceHandle();
		wndcls.hCursor = theApp.LoadStandardCursor(IDC_ARROW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = cls;
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(SideSection subclass) failed");
		reg.insert(cls);
	}

	return CWnd::Create(cls, NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, CRect(0, 0, 100, 100), parent, ID);
}

int SideSection::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) < 0) return -1;

	START_CREATE;
	CREATE(header, headerString(), sectionStyle);
	header.SetCheck(TRUE);
	header.SetOwner(this);
	header.ShowWindow(SW_SHOW);

	return 0;
}

void SideSection::Update(bool full)
{
	if (!full) return;
	CRect bounds; GetWindowRect(bounds);
	DS0;
	header.MoveWindow(0, 0, bounds.Width(), DS(20));
}

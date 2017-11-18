#include "stdafx.h"
#include "CPlotApp.h"
#include "Document.h"
#include "MainView.h"
#include "res/resource.h"
#include "MainWindow.h"
#include "SideView.h"
#include "Controls/ViewUtil.h"
#include "../Engine/Namespace/Expression.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum
{
	ID_f1 = 1000, ID_f2, ID_f3,
	ID_fs1, ID_fs2, ID_fs3,
	ID_domain, ID_coord, ID_mode,
	ID_plotView,
	ID_error
};

BEGIN_MESSAGE_MAP(BGStatic, CStatic)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
BOOL BGStatic::Create(CWnd *parent, int h, UINT ID, DWORD style)
{
	DS0;
	if (!CStatic::Create(_T(""), WS_CHILD | style, CRect(0, 0, 20, DS(h)), parent, ID)) return false;
	SetFont(&controlFont());
	return true;
}
BOOL BGStatic::OnEraseBkgnd(CDC *dc)
{
	RECT bounds; GetClientRect(&bounds);
	CBrush b(bg);
	dc->FillRect(&bounds, &b);
	return TRUE;
}


IMPLEMENT_DYNCREATE(MainView, CWnd)
BEGIN_MESSAGE_MAP(MainView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(ID_domain, OnDomainChange)
	ON_CBN_SELCHANGE(ID_coord, OnCoordChange)
	ON_CBN_SELCHANGE(ID_mode, OnModeChange)
END_MESSAGE_MAP()

HBRUSH MainView::OnCtlColor(CDC *dc, CWnd *wnd, UINT ctrl)
{
	if (ctrl == CTLCOLOR_STATIC)
	{
		COLORREF bg = plotView.GetBgColor();
		int r = GetRValue(bg), g = GetGValue(bg), b = GetBValue(bg);
		int y = (int)(0.21*r + 0.72*g + 0.07*b);
		bool light = (y > 160);
		dc->SetTextColor(GREY(light ? 0 : 255));
		dc->SetBkMode(MFC_TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return CWnd::OnCtlColor(dc, wnd, ctrl);
}
BOOL MainView::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CWnd::PreCreateWindow(cs)) return FALSE;

	cs.style &= ~WS_BORDER;
	cs.style |= WS_CHILD | WS_CLIPCHILDREN;
	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	return TRUE;
}

int MainView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) < 0) return -1;
	EnableScrollBarCtrl(SB_BOTH, FALSE);

	START_CREATE;

	POPUP(domain);
	OPTION("R \u2192 R");             DATA(R_R);
	OPTION("R\u00b2 \u2192 R");       DATA(R2_R);
	OPTION("C \u2192 C");             DATA(C_C);

	OPTION("R \u2192 R\u00b2");       DATA(R_R2);
	OPTION("R \u2192 R\u00b3");       DATA(R_R3);
	OPTION("S\u00b9 \u2192 R\u00b2"); DATA(S1_R2);
	OPTION("S\u00b9 \u2192 R\u00b3"); DATA(S1_R3);

	OPTION("R\u00b2 \u2192 R\u00b3"); DATA(R2_R3);
	OPTION("S\u00b2 \u2192 R\u00b3"); DATA(S2_R3);

	OPTION("R\u00b3 \u2192 R");       DATA(R3_R);
	OPTION("R\u00b2 \u2192 R\u00b2"); DATA(R2_R2);
	OPTION("R\u00b3 \u2192 R\u00b3"); DATA(R3_R3);

	POPUP(coord);

	POPUP(mode);

	#define FN(i) CREATEI( f[i-1], 20, ID_f ##i, editStyle)
	fs[0].Create(this, 20, ID_fs1, SS_RIGHT | SS_WORDELLIPSIS); FN(1);
	fs[1].Create(this, 20, ID_fs2, SS_RIGHT | SS_WORDELLIPSIS); FN(2);
	fs[2].Create(this, 20, ID_fs3, SS_RIGHT | SS_WORDELLIPSIS); FN(3);
	#undef FN
	
	f[0].OnChange = [this] { OnChangeF1(); };
	f[1].OnChange = [this] { OnChangeF2(); };
	f[2].OnChange = [this] { OnChangeF3(); };

	error.Create(this, 20, ID_error, SS_WORDELLIPSIS);

	plotView.Create(CRect(0,0,80,20), this, ID_plotView);

	return 0;
}

void MainView::Update()
{
	CRect bounds; GetWindowRect(bounds);
	if (bounds.Width() < 2) return;
	Document &doc = GetDocument();

	const Plot &plot = doc.plot;
	const Graph *graph = plot.current_graph();
	const bool sel = graph != NULL;
	
	doc.plot.axis.type(plot.axis_type());

	DS0;
	Layout layout(*this, 5, 22);
	SET(40, -1, -1, -1); if (layout[1] > DS(160)) { SET(40, 160, 160, -1); }

	//----------------------------------------------------------------------------------
	if (!graph)
	{
		HIDE(domain);
		HIDE(coord);
		HIDE(mode);
		HIDE(error);
	}
	else
	{
		USE(NULL, &domain, &coord, &mode);
		domain.EnableWindow(sel);
		coord.EnableWindow(sel);
		mode.EnableWindow(sel);

		for (int i = 0, n = domain.GetCount(); i < n; ++i)
		{
			if (domain.GetItemData(i) == (DWORD_PTR)graph->type())
			{
				domain.SetCurSel(i);
				break;
			}
		}

		int i = 0, i0 = -1;
		coord.ResetContent();
		for (GraphCoords c : graph->valid_coords())
		{
			CString label;
			switch (c)
			{
				case GC_Cartesian:   label = _T("Cartesian"); break;
				case GC_Polar:       label = _T("Polar"); break;
				case GC_Spherical:   label = _T("Spherical"); break;
				case GC_Cylindrical: label = _T("Cylindrical"); break;
				default: assert(false); continue;
			}
			if (c == graph->coords()) i0 = i;
			coord.InsertString(i, label);
			coord.SetItemData(i++, (DWORD_PTR)c);
		}
		coord.SetCurSel(i0);

		i = 0; i0 = -1;
		mode.ResetContent();
		for (GraphMode m : graph->valid_modes())
		{
			CString label;
			switch (m)
			{
				case GM_Graph:        label = _T("Graph"); break;
				case GM_Image:        label = _T("Image"); break;
				case GM_Riemann:      label = _T("Riemann Image"); break;
				case GM_VF:           label = _T("Vector Field"); break;
				case GM_Re:           label = _T("Real Part"); break;
				case GM_Im:           label = _T("Imaginary Part"); break;
				case GM_Abs:          label = _T("Absolute Value"); break;
				case GM_Phase:        label = _T("Phase"); break;
				case GM_Implicit:     label = _T("Implicit"); break;
				case GM_Color:        label = _T("Color"); break;
				case GM_RiemannColor: label = _T("Riemann Color"); break;
				case GM_Histogram:    label = _T("Histogram"); break;
				default: assert(false); continue;
			}
			if (m == graph->mode()) i0 = i;
			mode.InsertString(i, label);
			mode.SetItemData(i++, (DWORD_PTR)m);
		}
		mode.SetCurSel(i0);

		layout.skip(1);
	}

	int nf = graph ? graph->n_components() : 0; assert(nf >= 0 && nf <= 3);
	if (nf)
	{
		auto comp = graph->components(); assert(nf == (int)comp.size());
		for (int i = 0; i < nf; ++i)
		{
			USE(&fs[i], &f[i], &f[i], &f[i]);
			CString label = Convert(comp[i]); label.Append(_T(" ="));
			fs[i].SetWindowText(label);
			f[i].SetWindowText(Convert(graph->fn(i + 1)));
			fs[i].bg = plotView.GetBgColor();
			fs[i].Invalidate();
		}
	}
	for (int i = nf; i < 3; ++i)
	{
		HIDE(f[i]);
		HIDE(fs[i]);
	}
	layout.skip();

	// parsing errors
	if (graph)
	{
		bool show = false;
		Expression *expr = graph->expression();
		if (!expr)
		{
			if (plot.number_of_visible_graphs() > 0 && !plot.axis.valid())
			{
				error.SetWindowText(_T("Axis type mismatch"));
				show = true;
			}
		}
		else
		{
			auto &result = expr->result();
			int nf = graph->n_components();
			int i = (int)result.index;
			if (!result.ok && i < nf && !graph->fn(i + 1).empty())
			{
				error.SetWindowText(Convert(result.info));
				show = true;
				// TODO: mark the error position somehow
			}
		}

		if (!show)
		{
			HIDE(error);
		}
		else
		{
			error.bg = plotView.GetBgColor();
			USE(NULL, &error, &error, &error);
			layout.skip();
		}
	}

	plotView.MoveWindow(0, layout.y, layout.W, bounds.Height() - layout.y, 0);
	plotView.ShowWindow(SW_SHOW);

	RedrawHeader();
}

void MainView::OnInitialUpdate()
{
	SetClassLongPtr(*this, GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(BLACK_BRUSH));
	Update();
}

CRect MainView::headerRect() const
{
	CRect bounds; GetClientRect(&bounds);
	CRect rect; plotView.GetWindowRect(&rect); ScreenToClient(&rect);
	bounds.bottom = rect.top;
	return bounds;
}

void MainView::RedrawHeader()
{
	auto c = plotView.GetBgColor();
	for (int i = 0; i < 3; ++i)
	{
		fs[i].bg = c;
		fs[i].Invalidate();
	}
	error.bg = c;
	error.Invalidate();
	domain.Invalidate();
	coord.Invalidate();
	mode.Invalidate();
	InvalidateRect(headerRect());
}

BOOL MainView::OnEraseBkgnd(CDC *dc)
{
	CRect r = headerRect();
	CBrush bg(plotView.GetBgColor());
	dc->FillRect(r, &bg);
	return TRUE;
}

void MainView::OnSize(UINT type, int w, int h)
{
	CWnd::OnSize(type, w, h);
	EnableScrollBarCtrl(SB_BOTH, FALSE);
	Update();
	plotView.Invalidate();
}

void MainView::OnChangeF1() { CString s; f[0].GetWindowText(s); OnChangeF(1, s); }
void MainView::OnChangeF2() { CString s; f[1].GetWindowText(s); OnChangeF(2, s); }
void MainView::OnChangeF3() { CString s; f[2].GetWindowText(s); OnChangeF(3, s); }
void MainView::OnChangeF(int i, const CString &s_)
{
	Document &doc = GetDocument();
	Graph *g = doc.plot.current_graph(); if (!g) return;

	std::string s = Convert(s_);
	if (s == g->fn(i)) return;
	g->set(s, i);
	Update(); // errors might have changed
	RedrawHeader();
	plotView.Invalidate();
	GetSideView().UpdateGraphs(false);
}

void MainView::OnDomainChange()
{
	Document &doc = GetDocument();
	Graph *g = doc.plot.current_graph(); if (!g) return;

	int i = domain.GetCurSel();
	GraphType t = (GraphType)domain.GetItemData(i);
	if (t == g->type()) return;

	g->type(t);
	doc.plot.update_axis();
	Update();
	RedrawHeader();
	GetSideView().UpdateAll();
	plotView.Invalidate();
}

void MainView::OnCoordChange()
{
	Document &doc = GetDocument();
	Graph *g = doc.plot.current_graph(); if (!g) return;

	int i = coord.GetCurSel();
	GraphCoords c = (GraphCoords)coord.GetItemData(i);
	if (c == g->coords()) return;

	g->coords(c);
	Update();
	RedrawHeader();
	plotView.Invalidate();
}

void MainView::OnModeChange()
{
	Document &doc = GetDocument();
	Graph *g = doc.plot.current_graph(); if (!g) return;

	int i = mode.GetCurSel();
	GraphMode m = (GraphMode)mode.GetItemData(i);
	if (m == g->mode()) return;

	g->mode(m);
	doc.plot.update_axis();
	Update();
	RedrawHeader();
	GetSideView().UpdateAll();
	plotView.Invalidate();
}

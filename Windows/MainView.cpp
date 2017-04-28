#include "stdafx.h"
#include "CPlotApp.h"
#include "Document.h"
#include "MainView.h"
#include "res/resource.h"
#include "MainWindow.h"
#include "SideView.h"
#include "Controls/ViewUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum
{
	ID_f1 = 1000, ID_f2, ID_f3,
	ID_fs1, ID_fs2, ID_fs3,
	ID_domain, ID_coord, ID_mode,
	ID_plotView
};

IMPLEMENT_DYNCREATE(MainView, CFormView)
BEGIN_MESSAGE_MAP(MainView, CFormView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

MainView::MainView() : CFormView(IDD_MAINVIEW)
{
}

HBRUSH MainView::OnCtlColor(CDC *dc, CWnd *wnd, UINT ctrl)
{
	if (ctrl == CTLCOLOR_STATIC)
	{
		COLORREF bg = plotView.GetBgColor();
		int r = GetRValue(bg), g = GetGValue(bg), b = GetBValue(bg);
		int y = (int)(0.21*r + 0.72*g + 0.07*b);
		bool light = (y > 160);
		dc->SetTextColor(light ? RGB(0, 0, 0) : RGB(255, 255, 255));
		dc->SetBkMode(MFC_TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return CFormView::OnCtlColor(dc, wnd, ctrl);
}
BOOL MainView::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CFormView::PreCreateWindow(cs)) return FALSE;

	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	return TRUE;
}

int MainView::OnCreate(LPCREATESTRUCT cs)
{
	if (CFormView::OnCreate(cs) < 0) return -1;
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

	#define FS(i, s)   CREATEI(fs[i-1], ID_fs##i, _T(s), rlabelStyle)
	#define FN(i)      CREATEI( f[i-1], ID_f ##i,          editStyle)
	FS(1, "x ="); FN(1);
	FS(2, "y ="); FN(2);
	FS(3, "z ="); FN(3);
	#undef FS
	#undef FN
	
	plotView.Create(whatever, this, ID_plotView);

	return 0;
}

void MainView::Update()
{
	CRect bounds; GetClientRect(bounds);
	Document *doc = GetDocument();
	if (!doc || bounds.Width() < 2) return;

	const Plot &plot = doc->plot;
	const Graph *graph = plot.current_graph();
	const bool sel = graph != NULL;

	const int W = bounds.Width();
	const int SPC = 5; // amount of spacing
	const int w1 = 40; // label width
	int w2 = (W - SPC - w1 - 2 * SPC) / 3; if (w2 > 160) w2 = 160; // domain width
	const int x0 = SPC;  // row x start / amount of space on the left
	const int x1 = W - SPC;
	const int xm = x0 + w1, xmm = xm + SPC;
	int y = SPC; // y for next control

	const int h_label = 14, h_combo = 21, h_check = 20, h_edit = 20, h_row = 22;

	//----------------------------------------------------------------------------------
	if (!graph)
	{
		HIDE(domain);
		HIDE(coord);
		HIDE(mode);
	}
	else
	{
		domain.EnableWindow(sel);
		coord.EnableWindow(sel);
		mode.EnableWindow(sel);
		MOVE(domain, xmm, xmm + w2, y, h_combo, h_row);
		MOVE(coord, xmm + w2 + SPC, xmm + 2 * w2 + SPC, y, h_combo, h_row);
		MOVE(mode, xmm + 2 * (w2 + SPC), x1, y, h_combo, h_row);

		for (int i = 0, n = domain.GetCount(); i < n; ++i)
		{
			if (domain.GetItemData(i) == (DWORD_PTR)graph->type())
			{
				domain.SetCurSel(i);
				break;
			}
		}

		int i = 0, i0 = -1;
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
			coord.InsertString(i++, label);
		}
		coord.SetCurSel(i0);

		i = 0; i0 = -1;
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
			mode.InsertString(i++, label);
		}
		mode.SetCurSel(i0);

		y += h_row + 4;
	}

	int nf = graph ? graph->n_components() : 0; assert(nf >= 0 && nf <= 3);
	if (nf)
	{
		auto comp = graph->components(); assert(nf == (int)comp.size());
		for (int i = 0; i < nf; ++i)
		{
			MOVE(fs[i], x0, xm, y, h_label, h_row);
			MOVE(f[i], xmm, x1, y, h_edit, h_row);
			CString label = Convert(comp[i]); label.Append(_T(" ="));
			fs[i].SetWindowText(label);
			f[i].SetWindowText(Convert(graph->fn(i + 1)));
			y += h_row;
		}
	}
	for (int i = nf; i < 3; ++i)
	{
		HIDE(f[i]);
		HIDE(fs[i]);
	}
	y += SPC;

	plotView.ShowWindow(SW_SHOW);
	plotView.MoveWindow(0, y, W, bounds.Height() - y, 0);
}

void MainView::OnInitialUpdate()
{
	Update();
	CFormView::OnInitialUpdate();
}

BOOL MainView::OnEraseBkgnd(CDC *dc)
{
	CRect bounds; GetClientRect(&bounds);
	CRect rect; plotView.GetWindowRect(&rect); ScreenToClient(&rect);
	bounds.bottom = rect.top;

	COLORREF bgc = plotView.GetBgColor();
	CBrush bg(bgc);
	dc->FillRect(&bounds, &bg);
	return TRUE;
}

void MainView::OnSize(UINT type, int w, int h)
{
	CFormView::OnSize(type, w, h);
	EnableScrollBarCtrl(SB_BOTH, FALSE);
	Update();
	plotView.Invalidate();
}

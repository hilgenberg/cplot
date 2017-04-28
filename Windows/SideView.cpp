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
	ID_parameters = 1050,
	ID_definitions, ID_graphs,
	ID_settings,
	ID_qualityLabel, ID_quality,
	ID_discoLabel, ID_disco,
	ID_displayModeLabel, ID_displayMode,
	ID_colorLabel1, ID_colorLabel2,
	ID_bgColor, ID_axisColor, ID_gridColor, ID_fillColor,
	ID_textureLabel, ID_reflectionLabel,
	ID_texture, ID_reflection,
	ID_textureStrength, ID_reflectionStrength,
	ID_riemannTexture,
	ID_drawAxis,

	ID_axis,
	ID_center, ID_range,
	ID_xLabel, ID_xCenter, ID_xRange, ID_xDelta,
	ID_yLabel, ID_yCenter, ID_yRange, ID_yDelta,
	ID_rangeDelta

	// edit controls with formula support:
};

IMPLEMENT_DYNCREATE(SideView, CFormView)
BEGIN_MESSAGE_MAP(SideView, CFormView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEHWHEEL()
	ON_BN_CLICKED(ID_settings, OnSettings)
	ON_BN_CLICKED(ID_axis, OnAxis)
	ON_BN_CLICKED(ID_disco, OnDisco)
	ON_BN_CLICKED(ID_drawAxis, OnDrawAxis)
	//ON_CONTROL_RANGE(EN_KILLFOCUS, ID_XCENTER, ID_YRANGE, OnTextUpdate)
END_MESSAGE_MAP()

SideView::SideView()
: CFormView(IDD_SIDEVIEW)
, doc(NULL)
{
}
SideView::~SideView()
{
}

void SideView::SetDoc(Document *d)
{
	doc = d;
}

void SideView::Redraw()
{
	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().GetPlotView().Invalidate();
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

int SideView::OnCreate(LPCREATESTRUCT cs)
{
	if (CFormView::OnCreate(cs) < 0) return -1;

	EnableScrollBarCtrl(SB_HORZ, FALSE);

	START_CREATE;

	SECTION(parameters, "Parameters"); parameters.SetCanAdd(HeaderControl::Active);
	SECTION(definitions, "Definitions"); definitions.SetCanAdd(HeaderControl::Active);
	SECTION(graphs, "Graphs"); graphs.SetCanAdd(HeaderControl::Active); graphs.SetCanRemove(HeaderControl::Inactive);

	SECTION(settings, "Settings");
	LABEL(qualityLabel, "Quality:");
	CREATE(quality, sliderStyle); quality.SetRange(0, 64000);

	LABEL(discoLabel, "Detect:");
	CREATE(disco, _T("Discontinuities"), checkStyle);

	LABEL(displayModeLabel, "Display Mode:");
	POPUP(displayMode);
	OPTION("Shaded");
	OPTION("Smooth");
	OPTION("Phong");

	LABEL(colorLabel1, "Background, Fill:"); COLOR(bgColor); COLOR(fillColor);
	LABEL(colorLabel2, "Axis, Grid Color:"); COLOR(axisColor); COLOR(gridColor);

	LABEL(textureLabel, "Texture");
	LABEL(reflectionLabel, "Reflection");
	CREATE(texture, _T(""), WS_CHILD);
	CREATE(reflection, _T(""), WS_CHILD);
	CREATE(textureStrength, vSliderStyle);
	CREATE(reflectionStrength, vSliderStyle);
	CREATE(riemannTexture, _T("Riemann"), checkStyle);
	CREATE(drawAxis, _T("Draw Axis"), checkStyle);

	//----------------------------------------------------------------------------------

	SECTION(axis, "Axis");

	LABEL(center, "Center");
	LABEL(range,  "Range");

	LABEL(xLabel, "x:");
	CREATE(xCenter, editStyle);
	CREATE(xRange,  editStyle);
	DELTA(xDelta);

	LABEL(yLabel, "y:");
	CREATE(yCenter, editStyle);
	CREATE(yRange,  editStyle);
	DELTA(yDelta);

	DELTA(rangeDelta);

	return 0;
}

void SideView::Update()
{
	CRect bounds; GetClientRect(bounds);
	if (!doc || bounds.Width() < 2) return;
	const Plot &plot = doc->plot;
	const Graph *graph = plot.current_graph();
	const bool sel = graph != NULL;

	const int W = bounds.Width();
	const int SPC = 5; // amount of spacing
	int y = 0; // y for next control
	const int x0 = SPC;  // row x start / amount of space on the left

	const int h_section = 20, h_label = 14, h_combo = 21, h_check = 20, 
	          h_edit = 20, h_color = 20, h_slider = 20, h_delta = h_slider, 
	          w_slider = h_slider, h_row = 24;

	//----------------------------------------------------------------------------------
	MOVE(parameters, 0, W, y, h_section, h_section); y += h_row-(h_row-h_section)/2;
	//----------------------------------------------------------------------------------
	MOVE(definitions, 0, W, y, h_section, h_row); y += h_row;
	//----------------------------------------------------------------------------------
	MOVE(graphs, 0, W, y, h_section, h_row); y += h_row;
	//----------------------------------------------------------------------------------
	MOVE(settings, 0, W, y, h_section, h_row); y += h_row;
	if (!settings.GetCheck())
	{
		HIDE(qualityLabel);     HIDE(quality);
		HIDE(discoLabel);       HIDE(disco);
		HIDE(displayModeLabel); HIDE(displayMode);
		HIDE(colorLabel1);      HIDE(colorLabel2);
		HIDE(bgColor); HIDE(axisColor); HIDE(gridColor); HIDE(fillColor);

		HIDE(textureLabel);
		HIDE(reflectionLabel);
		HIDE(texture);
		HIDE(textureStrength);
		HIDE(reflectionStrength);
		HIDE(reflection);
		HIDE(riemannTexture);
		HIDE(drawAxis);
	}
	else
	{
		const int w1 = 80; // label width
		const int x1 = W - SPC;
		const int xm = x0 + w1, xmm = xm + SPC;

		MOVE(qualityLabel, x0, xm, y, h_label, h_row);
		MOVE(quality, xmm, x1, y, h_slider, h_row);
		y += h_row;

		MOVE(discoLabel, x0, xm, y, h_label, h_row);
		MOVE(disco, xmm, x1, y, h_check, h_row);
		discoLabel.EnableWindow(sel);
		disco.EnableWindow(sel);
		disco.SetCheck(sel && graph->options.disco);
		y += h_row;

		MOVE(displayModeLabel, x0, xm, y, h_label, h_row);
		MOVE(displayMode, xmm, x1, y, h_combo, h_row);
		displayModeLabel.EnableWindow(sel);
		displayMode.EnableWindow(sel);
		displayMode.SetCurSel(sel ? 2 : 0);
		y += h_row;

		int d = (W - w1 - 4 * SPC) / 2;
		MOVE(colorLabel1, x0, xm, y, h_label, h_row);
		MOVE(bgColor, xmm, xmm+d, y, h_color, h_row);
		MOVE(fillColor, xmm+d+SPC, x1, y, h_color, h_row);
		y += h_row;
		MOVE(colorLabel2, x0, xm, y, h_label, h_row);
		MOVE(axisColor, xmm, xmm + d, y, h_color, h_row);
		MOVE(gridColor, xmm + d + SPC, x1, y, h_color, h_row);
		y += h_row + SPC;
		fillColor.EnableWindow(sel);
		gridColor.EnableWindow(sel);
		bgColor.SetColor(plot.axis.options.background_color);
		axisColor.SetColor(plot.axis.options.axis_color);
		if (graph)
		{
			fillColor.SetColor(graph->options.fill_color);
			gridColor.SetColor(graph->options.grid_color);
		}

		const int dt = (W - 9 * SPC - 2 * w_slider) / 2;
		const int t1 = x0 + 2*SPC, t2 = t1 + dt, t3 = W - 3 * SPC - dt, t4 = t3 + dt;
		MOVE(textureLabel, t1, t2, y, h_label, h_row);
		MOVE(reflectionLabel, t3, t4, y, h_label, h_row);
		y += h_row;
		MOVE(texture, t1, t2, y, dt, dt);
		MOVE(textureStrength, t2 + SPC, t2 + SPC + w_slider, y, dt, dt);
		MOVE(reflectionStrength, t3 - SPC - w_slider, t3 - SPC, y, dt, dt);
		MOVE(reflection, t3, t4, y, dt, dt);
		y += dt;
		MOVE(riemannTexture, t1+4, t2, y, h_check, h_row);
		y += h_row + SPC;
		MOVE(drawAxis, t1, t2+SPC+w_slider, y, h_check, h_row);
		drawAxis.SetCheck(!plot.axis.options.hidden);
		y += h_row;
	}
	y += SPC;
	//----------------------------------------------------------------------------------
	MOVE(axis, 0, W, y, h_section, h_row); y += h_row;
	if (!axis.GetCheck())
	{
		HIDE(center); HIDE(range);
		HIDE(xLabel); HIDE(xCenter); HIDE(xRange); HIDE(xDelta);
		HIDE(yLabel); HIDE(yCenter); HIDE(yRange); HIDE(yDelta);
		HIDE(rangeDelta);
	}
	else
	{
		const int w1 = 20; // label width
		const int x1 = W - SPC;
		const int xm = x0 + w1, xmm = xm + SPC;
		const int wd = 80;
		int d = (x1 - xmm - SPC - wd) / 2;
		int t1 = xmm, t2 = xmm + d, t3 = xmm + 2 * d + SPC;
		MOVE(center, t1, t2, y, h_label, h_row);
		MOVE(range, t2+SPC, t3, y, h_label, h_row);
		y += h_row - 5;

		MOVE(xLabel, x0, xm, y, h_label, h_row);
		MOVE(xCenter, t1, t2, y, h_edit, h_row);
		MOVE(xRange, t2+SPC, t3, y, h_edit, h_row);
		MOVE(xDelta, t3 + SPC, x1, y, h_delta, h_row);
		y += h_row;

		MOVE(yLabel, x0, xm, y, h_label, h_row);
		MOVE(yCenter, t1, t2, y, h_edit, h_row);
		MOVE(yRange, t2 + SPC, t3, y, h_edit, h_row);
		MOVE(yDelta, t3 + SPC, x1, y, h_delta, h_row);
		y += h_row;

		MOVE(rangeDelta, t1, t3, y, h_delta, h_row);
		y += h_row;
	}
	y += SPC;
	//----------------------------------------------------------------------------------

	EnableScrollBarCtrl(SB_HORZ, FALSE);
	SetScrollSizes(MM_TEXT, CSize(W, y), CSize(W, bounds.Height()), CSize(h_row, h_row));
}

void SideView::OnInitialUpdate()
{
	Update();
	CFormView::OnInitialUpdate();
}

BOOL SideView::OnEraseBkgnd(CDC *dc)
{
	RECT bounds; GetClientRect(&bounds);
	CBrush bg(GetSysColor(COLOR_BTNFACE));
	dc->FillRect(&bounds, &bg);
	return TRUE;
}

void SideView::OnDraw(CDC *dc)
{
	CFormView::OnDraw(dc);
}

void SideView::OnSize(UINT type, int w, int h)
{
	CFormView::OnSize(type, w, h);
	Update();
	Invalidate();
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

void SideView::OnSettings()
{
	Update();
}
void SideView::OnAxis()
{
	Update();
}

static inline void toggle(bool &b) { b = !b; }

void SideView::OnDisco()
{
	if (!doc) return;
	Graph *graph = doc->plot.current_graph();
	if (!graph) return;
	toggle(graph->options.disco);
	Update();
	Redraw();
}

void SideView::OnDrawAxis()
{
	if (!doc) return;
	toggle(doc->plot.axis.options.hidden);
	Update();
	Redraw();
}

void SideView::OnAdd(HeaderControl *sender)
{

}
void SideView::OnRemove(HeaderControl *sender)
{

}

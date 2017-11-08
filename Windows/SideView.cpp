#include "stdafx.h"
#include "CPlotApp.h"
#include "SideView.h"
#include "Document.h"
#include "Controls/SplitterWnd.h"
#include "MainWindow.h"
#include "MainView.h"
#include "Controls/ViewUtil.h"
#include "res/resource.h"
#include "SideView_IDs.h"

IMPLEMENT_DYNCREATE(SideView, CFormView)
BEGIN_MESSAGE_MAP(SideView, CFormView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEHWHEEL()
	ON_BN_CLICKED(ID_settings, OnSettings)
	ON_BN_CLICKED(ID_axis, OnAxis)

	ON_BN_CLICKED(ID_disco, OnDisco)
	ON_CBN_SELCHANGE(ID_displayMode, OnDisplayMode)
	ON_CBN_SELCHANGE(ID_vfMode, OnVFMode)
	ON_CBN_SELCHANGE(ID_histoMode, OnHistoMode)
	ON_CBN_SELCHANGE(ID_aaMode, OnAAMode)
	ON_CBN_SELCHANGE(ID_transparencyMode, OnTransparencyMode)
	ON_BN_CLICKED(ID_bgColor,   OnBgColor)
	ON_BN_CLICKED(ID_fillColor, OnFillColor)
	ON_BN_CLICKED(ID_axisColor, OnAxisColor)
	ON_BN_CLICKED(ID_gridColor, OnGridColor)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(ID_textureMode, OnTextureMode)
	ON_CBN_SELCHANGE(ID_gridMode, OnGridMode)
	ON_CBN_SELCHANGE(ID_meshMode, OnMeshMode)
	ON_BN_CLICKED(ID_drawAxis, OnDrawAxis)
	ON_BN_CLICKED(ID_clip, OnClip)
	ON_CBN_SELCHANGE(ID_axisMode, OnAxisMode)
	ON_BN_CLICKED(ID_clipCustom, OnClipCustom)
	ON_BN_CLICKED(ID_clipLock,   OnClipLock)
	ON_BN_CLICKED(ID_clipReset,  OnClipReset)
	ON_BN_CLICKED(ID_center, OnCenterAxis)
	ON_BN_CLICKED(ID_top,    OnTopView)
	ON_BN_CLICKED(ID_front,  OnFrontView)
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

void SideView::UpdateAxis()
{
	// TODO: only reset the axis items
	//Update();
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

//---------------------------------------------------------------------------------------------
// Headers
//---------------------------------------------------------------------------------------------

BoxState SideView::GetBoxState() const
{
	BoxState b; b.all = 0;
	b.params = parameters.GetCheck();
	b.defs = definitions.GetCheck();
	b.graphs = graphs.GetCheck();
	b.settings = settings.GetCheck();
	b.axis = axis.GetCheck();
	return b;
}

void SideView::SetBoxState(BoxState b)
{
	parameters.SetCheck(b.params);
	definitions.SetCheck(b.defs);
	graphs.SetCheck(b.graphs);
	settings.SetCheck(b.settings);
	axis.SetCheck(b.axis);
	Update();
}

void SideView::OnSettings()
{
	Update();
}

void SideView::OnAxis()
{
	Update();
}

//---------------------------------------------------------------------------------------------
// Check Boxes & Buttons
//---------------------------------------------------------------------------------------------

static inline void toggle(bool &b) { b = !b; }

void SideView::OnDisco()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;
	toggle(g->options.disco);
	disco.SetCheck(g->options.disco ? BST_CHECKED : BST_UNCHECKED);
	Recalc(g);
}

void SideView::OnDrawAxis()
{
	if (!doc) return;
	toggle(doc->plot.axis.options.hidden);
	drawAxis.SetCheck(!doc->plot.axis.options.hidden ? BST_CHECKED : BST_UNCHECKED);
	Redraw();
}

void SideView::OnClip()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	g->clipping(!g->clipping());
	clip.SetCheck(g->clipping() ? BST_CHECKED : BST_UNCHECKED);
	Recalc(g);
}
void SideView::OnClipCustom()
{
	if (!doc) return;
	Plot &plot = doc->plot;
	plot.options.clip.on(!plot.options.clip.on());
	clipCustom.SetCheck(plot.options.clip.on() ? BST_CHECKED : BST_UNCHECKED);
	Update();
	Redraw();
}
void SideView::OnClipLock()
{
	if (!doc) return;
	Plot &plot = doc->plot;
	plot.options.clip.locked(!plot.options.clip.locked());
	clipLock.SetCheck(plot.options.clip.locked() ? BST_CHECKED : BST_UNCHECKED);
	Update();
	Redraw();
}
void SideView::OnClipReset()
{
	if (!doc) return;
	Plot &plot = doc->plot;
	plot.options.clip.normal(plot.camera.view_vector());
	Recalc(plot);
}

void SideView::OnCenterAxis()
{
	if (!doc) return;
	Plot &plot = doc->plot;
	plot.axis.reset_center();
	Update();
	Recalc(plot);
}

void SideView::OnEqualRanges()
{
	if (!doc) return;
	Plot &plot = doc->plot;
	plot.axis.equal_ranges();
	Update();
	Recalc(plot);
}

void SideView::ChangeView(const P3d &v)
{
	if (!doc) return;
	Plot &plot = doc->plot;
	Camera &camera = plot.camera;
	camera.set_angles(v.x, v.y, v.z);
	Update();
	Redraw();
}

void SideView::OnAdd(HeaderControl *sender)
{

}
void SideView::OnRemove(HeaderControl *sender)
{

}

//---------------------------------------------------------------------------------------------
// Sliders
//---------------------------------------------------------------------------------------------

void SideView::OnHScroll(UINT code, UINT pos, CScrollBar *sb)
{
	if (!doc)
	{
		CFormView::OnHScroll(code, pos, sb);
		return;
	}

	CSliderCtrl *sc = (CSliderCtrl*)sb;
	if (sc == &quality)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		g->options.quality = quality.GetPos() / (double)SLIDER_MAX;
		Recalc(g);
	}
	else if (sc == &bgAlpha)
	{
		doc->plot.axis.options.background_color.SetAlpha(bgAlpha.GetPos());
		Redraw();
	}
	else if (sc == &axisAlpha)
	{
		doc->plot.axis.options.axis_color.SetAlpha(axisAlpha.GetPos());
		Redraw();
	}
	else if (sc == &fillAlpha)
	{
		Graph *g = doc->plot.current_graph(); if (!g || !g->hasFill()) return;
		g->options.fill_color.SetAlpha(fillAlpha.GetPos());
		Redraw();
	}
	else if (sc == &gridAlpha)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		(g->usesLineColor() ? g->options.line_color : g->options.grid_color).SetAlpha(gridAlpha.GetPos());
		Redraw();
	}
	else if (sc == &fog)
	{
		doc->plot.options.fog = fog.GetPos() / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &lineWidth)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		(g->usesLineColor() ? g->options.line_width : g->options.gridline_width) = lineWidth.GetPos() / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &shinyness)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		g->options.shinyness = shinyness.GetPos() / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &gridDensity)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		g->options.grid_density = sc->GetPos() / (double)SLIDER_MAX;
		Recalc(g);
	}
	else if (sc == &meshDensity)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		g->options.mask.density(sc->GetPos() / (double)SLIDER_MAX);
		Redraw();
	}
	else if (sc == &histoScaleSlider)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		double x = std::max(0.0, sc->GetPos() / (double)SLIDER_MAX);
		g->options.hist_scale = (exp(2.0*x*log(HISTO_MAX - 1.0)) - 1.0) / (HISTO_MAX - 2.0);
		Update();
		Recalc(g);
	}
	else if (sc == &clipDistance)
	{
		doc->plot.options.clip.distance(sc->GetPos() / (float)SLIDER_MAX);
		Redraw();
	}
	else
	{
		CFormView::OnHScroll(code, pos, sb);
	}
}

void SideView::OnVScroll(UINT code, UINT pos, CScrollBar *sb)
{
	if (!doc)
	{
		CFormView::OnVScroll(code, pos, sb);
		return;
	}

	CSliderCtrl *sc = (CSliderCtrl*)sb;
	if (sc == &textureStrength)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		g->options.texture_opacity = (SLIDER_MAX - textureStrength.GetPos()) / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &reflectionStrength)
	{
		Graph *g = doc->plot.current_graph(); if (!g) return;
		g->options.reflection_opacity = (SLIDER_MAX - reflectionStrength.GetPos()) / (double)SLIDER_MAX;
		Redraw();
	}
	else
	{
		CFormView::OnVScroll(code, pos, sb);
	}
}

//---------------------------------------------------------------------------------------------
// Edit Fields
//---------------------------------------------------------------------------------------------

void SideView::OnHistoScale()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;
	double x = histoScale.GetDouble();
	if (defined(x)) g->options.hist_scale = std::max(0.0, x);

	Update();
	Recalc(g);
}

void SideView::OnAxisRange(int i, NumericEdit &e)
{
	if (!doc) return;
	Plot &plot = doc->plot;
	double x = e.GetDouble() * 0.5;
	if (!defined(x)) return;
	if (i < 0)
	{
		i = -(i+1);
		plot.axis.in_range(i, x);
	}
	else
	{
		plot.axis.range(i, x);
	}
	Update();
	Recalc(plot);
}

void SideView::OnAxisCenter(int i, NumericEdit &e)
{
	if (!doc) return;
	Plot &plot = doc->plot;
	double x = e.GetDouble();
	if (!defined(x)) return;
	if (i < 0)
	{
		i = -(i + 1);
		plot.axis.in_center(i, x);
	}
	else
	{
		plot.axis.center(i, x);
	}
	Update();
	Recalc(plot);
}

void SideView::OnAxisAngle(int i, NumericEdit &e)
{
	if (!doc) return;
	Camera &cam = doc->plot.camera;
	double x = e.GetDouble();
	if (!defined(x)) return;
	switch (i)
	{
		case 0: cam.set_phi(x); break;
		case 1: cam.set_psi(x); break;
		case 2: cam.set_theta(x); break;
		default: assert(false); break;
	}
	Update();
	Redraw();
}

void SideView::OnDistance()
{
	if (!doc) return;
	Camera &cam = doc->plot.camera;
	double x = dist.GetDouble();
	if (!defined(x) || x <= 0.0) return;
	cam.set_zoom(1.0 / x);
	Update();
	Redraw();
}

//---------------------------------------------------------------------------------------------
// Combo Boxes
//---------------------------------------------------------------------------------------------

void SideView::OnDisplayMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = displayMode.GetCurSel();
	ShadingMode m = (ShadingMode)displayMode.GetItemData(i);
	if (m == g->options.shading_mode) return;

	g->options.shading_mode = m;
	doc->plot.update_axis();
	Update();
	Recalc(g);
}

void SideView::OnVFMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = vfMode.GetCurSel();
	VectorfieldMode m = (VectorfieldMode)vfMode.GetItemData(i);
	if (m == g->options.vf_mode) return;

	g->options.vf_mode = m;
	Recalc(g);
}

void SideView::OnHistoMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = histoMode.GetCurSel();
	HistogramMode m = (HistogramMode)histoMode.GetItemData(i);
	if (m == g->options.hist_mode) return;

	g->options.hist_mode = m;
	doc->plot.update_axis();

	Update();
	Recalc(g);
}

void SideView::OnAAMode()
{
	if (!doc) return;
	Plot &plot = doc->plot;

	int i = aaMode.GetCurSel();
	AntialiasMode m = (AntialiasMode)aaMode.GetItemData(i);
	if (m == plot.options.aa_mode) return;

	plot.options.aa_mode = m;
	Redraw();
}

void SideView::OnTransparencyMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = transparencyMode.GetCurSel();
	auto &tm = DefaultBlendModes();
	int n = (int)tm.size();
	if (i >= 0 && i < n)
	{
		if (g->options.transparency == tm[i].mode) return;
		g->options.transparency = tm[i].mode;
		if (i == 0)
		{
			// [self.customTransparency hideForGraph : g];
		}
		else
		{
			// [self.customTransparency updateForGraph : g];
		}
	}
	else
	{
		//[self.customTransparency runForGraph : g];
		Update();
		return;
	}

	Redraw();
}

void SideView::OnGridMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = gridMode.GetCurSel();
	GridStyle m = (GridStyle)gridMode.GetItemData(i);
	if (m == g->options.grid_style) return;

	g->options.grid_style = m;
	Update();
	Recalc(g);
}

void SideView::OnMeshMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = meshMode.GetCurSel();
	MaskStyle m = (MaskStyle)meshMode.GetItemData(i);
	if (m == g->options.mask.style()) return;

	if (m == Mask_Custom) return;

	g->options.mask.style(m);
	Update();
	Redraw();
}

void SideView::OnAxisMode()
{
	if (!doc) return;
	Plot &plot = doc->plot;

	int i = axisMode.GetCurSel();
	AxisOptions::AxisGridMode m = (AxisOptions::AxisGridMode)gridMode.GetItemData(i);
	if (m == plot.axis.options.axis_grid) return;

	plot.axis.options.axis_grid = m;
	Redraw();
}

void SideView::OnTextureMode()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	int i = textureMode.GetCurSel();
	TextureProjection m = (TextureProjection)textureMode.GetItemData(i);
	if (m == g->options.texture_projection) return;

	g->options.texture_projection = m;
	Recalc(g);
}

//---------------------------------------------------------------------------------------------
// Textures & Colors
//---------------------------------------------------------------------------------------------

void SideView::OnChangeTexture(int i)
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;

	g->isColor() ? Recalc(g) : Redraw();
}

void SideView::OnBgColor()
{
	if (!doc) return;
	doc->plot.axis.options.background_color = bgColor.GetColor();
	Redraw();

	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().RedrawHeader();
}
void SideView::OnAxisColor()
{
	if (!doc) return;
	doc->plot.axis.options.axis_color = axisColor.GetColor();
	Redraw();
}
void SideView::OnFillColor()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g || !g->hasFill()) return;
	g->options.fill_color = fillColor.GetColor();
	Redraw();
}
void SideView::OnGridColor()
{
	if (!doc) return;
	Graph *g = doc->plot.current_graph(); if (!g) return;
	(g->usesLineColor() ? g->options.line_color : g->options.grid_color) = gridColor.GetColor();
	Redraw();
}

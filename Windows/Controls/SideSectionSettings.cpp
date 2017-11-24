#include "../stdafx.h"
#include "SideSectionSettings.h"
#include "ViewUtil.h"
#include "../res/resource.h"
#include "../DefinitionController.h"
#include "DefinitionView.h"
#include "../../Graphs/Plot.h"
#include "../SideView.h"
#include "../Document.h"
#include "../MainWindow.h"
#include "../MainView.h"

enum
{
	ID_header = 2000,

	ID_qualityLabel, ID_quality,
	ID_discoLabel, ID_disco,
	ID_displayModeLabel, ID_displayMode, ID_vfMode,
	ID_histoModeLabel, ID_histoMode,
	ID_histoScaleLabel, ID_histoScale, ID_histoScaleSlider,

	ID_gridModeLabel, ID_meshModeLabel,
	ID_gridMode, ID_meshMode,
	ID_gridDensity, ID_meshDensity,

	ID_drawAxis, ID_axisModeLabel,
	ID_clip, ID_axisMode,
	ID_clipCustom, ID_clipLock, ID_clipReset,
	ID_clipDistance,
};

BEGIN_MESSAGE_MAP(SideSectionSettings, SideSection)
	ON_WM_CREATE()
	ON_BN_CLICKED(ID_disco, OnDisco)
	ON_CBN_SELCHANGE(ID_displayMode, OnDisplayMode)
	ON_CBN_SELCHANGE(ID_vfMode, OnVFMode)
	ON_CBN_SELCHANGE(ID_histoMode, OnHistoMode)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(ID_gridMode, OnGridMode)
	ON_CBN_SELCHANGE(ID_meshMode, OnMeshMode)
	ON_BN_CLICKED(ID_drawAxis, OnDrawAxis)
	ON_BN_CLICKED(ID_clip, OnClip)
	ON_CBN_SELCHANGE(ID_axisMode, OnAxisMode)
	ON_BN_CLICKED(ID_clipCustom, OnClipCustom)
	ON_BN_CLICKED(ID_clipLock, OnClipLock)
	ON_BN_CLICKED(ID_clipReset, OnClipReset)
END_MESSAGE_MAP()

static constexpr int    SLIDER_MAX = 64000;
static constexpr double HISTO_MAX  = 1.0e4;

//---------------------------------------------------------------------------------------------
// Check Boxes & Buttons
//---------------------------------------------------------------------------------------------

static inline void toggle(bool &b) { b = !b; }

void SideSectionSettings::OnDisco()
{
	Graph *g = document().plot.current_graph(); if (!g) return;
	toggle(g->options.disco);
	disco.SetCheck(g->options.disco ? BST_CHECKED : BST_UNCHECKED);
	Recalc(g);
}

void SideSectionSettings::OnDrawAxis()
{
	toggle(document().plot.axis.options.hidden);
	drawAxis.SetCheck(!document().plot.axis.options.hidden ? BST_CHECKED : BST_UNCHECKED);
	Redraw();
}

void SideSectionSettings::OnClip()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	g->clipping(!g->clipping());
	clip.SetCheck(g->clipping() ? BST_CHECKED : BST_UNCHECKED);
	Recalc(g);
}
void SideSectionSettings::OnClipCustom()
{
	Plot &plot = document().plot;
	plot.options.clip.on(!plot.options.clip.on());
	clipCustom.SetCheck(plot.options.clip.on() ? BST_CHECKED : BST_UNCHECKED);
	Update(false);
	Redraw();
}
void SideSectionSettings::OnClipLock()
{
	Plot &plot = document().plot;
	plot.options.clip.locked(!plot.options.clip.locked());
	clipLock.SetCheck(plot.options.clip.locked() ? BST_CHECKED : BST_UNCHECKED);
	Update(false);
	Redraw();
}
void SideSectionSettings::OnClipReset()
{
	Plot &plot = document().plot;
	plot.options.clip.normal(plot.camera.view_vector());
	Recalc(plot);
}

void SideSectionSettings::OnToggleGrid()
{
	Graph *g = document().plot.current_graph(); if (!g) return;
	g->options.grid_style = (g->options.grid_style == Grid_Off ? Grid_On : Grid_Off);
	Update(false); // enable slider
	Recalc(g);
}

//---------------------------------------------------------------------------------------------
// Sliders
//---------------------------------------------------------------------------------------------

void SideSectionSettings::OnHScroll(UINT code, UINT pos, CScrollBar *sb)
{
	CSliderCtrl *sc = (CSliderCtrl*)sb;
	if (sc == &quality)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		g->options.quality = quality.GetPos() / (double)SLIDER_MAX;
		Recalc(g);
	}
	else if (sc == &gridDensity)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		g->options.grid_density = sc->GetPos() / (double)SLIDER_MAX;
		Recalc(g);
	}
	else if (sc == &meshDensity)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		g->options.mask.density(sc->GetPos() / (double)SLIDER_MAX);
		Redraw();
	}
	else if (sc == &histoScaleSlider)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		double x = std::max(0.0, sc->GetPos() / (double)SLIDER_MAX);
		g->options.hist_scale = (exp(2.0*x*log(HISTO_MAX - 1.0)) - 1.0) / (HISTO_MAX - 2.0);
		Update(false); // update the edit field
		Recalc(g);
	}
	else if (sc == &clipDistance)
	{
		document().plot.options.clip.distance(sc->GetPos() / (float)SLIDER_MAX);
		Redraw();
	}
	else
	{
		SideSection::OnHScroll(code, pos, sb);
	}
}

//---------------------------------------------------------------------------------------------
// Edit Fields
//---------------------------------------------------------------------------------------------

void SideSectionSettings::OnHistoScale()
{
	Graph *g = document().plot.current_graph(); if (!g) return;
	double x = histoScale.GetDouble();
	if (defined(x)) g->options.hist_scale = std::max(0.0, x);

	Update(false); // update the slider
	Recalc(g);
}

//---------------------------------------------------------------------------------------------
// Combo Boxes
//---------------------------------------------------------------------------------------------

void SideSectionSettings::OnDisplayMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = displayMode.GetCurSel();
	ShadingMode m = (ShadingMode)displayMode.GetItemData(i);
	if (m == g->options.shading_mode) return;

	g->options.shading_mode = m;
	document().plot.update_axis();
	parent().UpdateSettings(true);
	Recalc(g);
}

void SideSectionSettings::OnVFMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = vfMode.GetCurSel();
	VectorfieldMode m = (VectorfieldMode)vfMode.GetItemData(i);
	if (m == g->options.vf_mode) return;

	g->options.vf_mode = m;
	Recalc(g);
}

void SideSectionSettings::OnCycleVFMode(int d)
{
	assert(d == 1 || d == -1);
	Graph *g = document().plot.current_graph(); if (!g) return;

	int m = d + (int)g->options.vf_mode;
	if (m < 0) m = VF_LAST;
	if (m > VF_LAST) m = 0;

	g->options.vf_mode = (VectorfieldMode)m;
	Recalc(g);
	Update(false);
}

void SideSectionSettings::OnHistoMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = histoMode.GetCurSel();
	HistogramMode m = (HistogramMode)histoMode.GetItemData(i);
	if (m == g->options.hist_mode) return;

	g->options.hist_mode = m;
	document().plot.update_axis();

	parent().UpdateSettings(true);
	Recalc(g);
}

void SideSectionSettings::OnGridMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = gridMode.GetCurSel();
	GridStyle m = (GridStyle)gridMode.GetItemData(i);
	if (m == g->options.grid_style) return;

	g->options.grid_style = m;
	Update(false); // enable slider
	Recalc(g);
}

void SideSectionSettings::OnMeshMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = meshMode.GetCurSel();
	MaskStyle m = (MaskStyle)meshMode.GetItemData(i);
	if (m == g->options.mask.style()) return;

	if (m == Mask_Custom) return;

	g->options.mask.style(m);
	Update(false); // enable slider
	Redraw();
}

void SideSectionSettings::OnAxisMode()
{
	Plot &plot = document().plot;

	int i = axisMode.GetCurSel();
	AxisOptions::AxisGridMode m = (AxisOptions::AxisGridMode)gridMode.GetItemData(i);
	if (m == plot.axis.options.axis_grid) return;

	plot.axis.options.axis_grid = m;
	Redraw();
}

//---------------------------------------------------------------------------------------------
// Create & Update
//---------------------------------------------------------------------------------------------

int SideSectionSettings::OnCreate(LPCREATESTRUCT cs)
{
	if (SideSection::OnCreate(cs) < 0) return -1;

	START_CREATE;

	LABEL(qualityLabel, "Quality:");
	SLIDER(quality, SLIDER_MAX / 10);

	LABEL(discoLabel, "Detect:");
	CHECK(disco, "Discontinuities");

	LABEL(displayModeLabel, "Display Mode:");
	POPUP(displayMode);
	OPTION("Points");     DATA(Shading_Points);
	OPTION("Wireframe");  DATA(Shading_Wireframe);
	OPTION("Hiddenline"); DATA(Shading_Hiddenline);
	OPTION("Flatshaded"); DATA(Shading_Flat);
	OPTION("Smooth");     DATA(Shading_Smooth);
	POPUP(vfMode);
	OPTION("Unscaled");   DATA(VF_Unscaled);
	OPTION("Normalized"); DATA(VF_Normalized);
	OPTION("Direction");  DATA(VF_Unit);
	OPTION("Connected");  DATA(VF_Connected);

	LABEL(histoModeLabel, "Histo Mode:");
	POPUP(histoMode);
	OPTION("Riemann"); DATA(HM_Riemann);
	OPTION("Disc");    DATA(HM_Disc);
	OPTION("Normal");  DATA(HM_Normal);
	LABEL(histoScaleLabel, "Histo Scale:");
	EDIT(histoScale);
	histoScale.OnChange = [this]() { OnHistoScale(); };
	SLIDER(histoScaleSlider, SLIDER_MAX);

	LABEL(gridModeLabel, "Grid");
	LABEL(meshModeLabel, "Mesh");
	POPUP(gridMode);
	OPTION("Off");  DATA(Grid_Off);
	OPTION("On");   DATA(Grid_On);
	OPTION("Full"); DATA(Grid_Full);
	POPUP(meshMode);
	OPTION("Off");          DATA(Mask_Off);
	OPTION("Chess");        DATA(Mask_Chessboard);
	OPTION("HLines");       DATA(Mask_HLines);
	OPTION("VLines");       DATA(Mask_VLines);
	OPTION("Circles");      DATA(Mask_Circles);
	OPTION("Squares");      DATA(Mask_Squares);
	OPTION("Triangles");    DATA(Mask_Triangles);
	OPTION("Rounded Rect"); DATA(Mask_Rounded_Rect);
	OPTION("Rings");        DATA(Mask_Rings);
	OPTION("Fan");          DATA(Mask_Fan);
	OPTION("Static");       DATA(Mask_Static);
	OPTION("Hexagon");      DATA(Mask_Hexagon);
	OPTION("Custom...");    DATA(Mask_Custom);
	SLIDER(gridDensity, SLIDER_MAX * 100);
	SLIDER(meshDensity, SLIDER_MAX);

	CHECK(drawAxis, "Draw Axis"); LABEL(axisModeLabel, "Axis Grid:");
	CHECK(clip, "Clip to Axis");
	POPUP(axisMode);
	OPTION("Off");       DATA(AxisOptions::AG_Off);
	OPTION("Cartesian"); DATA(AxisOptions::AG_Cartesian);
	OPTION("Polar");     DATA(AxisOptions::AG_Polar);

	CHECK(clipCustom, "Clip Custom");
	CHECK(clipLock, "Lock");
	BUTTON(clipReset, "Reset");
	SLIDER(clipDistance, SLIDER_MAX); clipDistance.SetRangeMin(-SLIDER_MAX);

	return 0;
}

static int find(CComboBox &b, DWORD_PTR itemData)
{
	for (int i = 0, n = b.GetCount(); i < n; ++i)
	{
		if (b.GetItemData(i) == itemData) return i;
	}
	return -1;
}

void SideSectionSettings::Update(bool full)
{
	SideSection::Update(full);
	CRect bounds; GetWindowRect(bounds);
	if (bounds.Width() < 2) return;

	const Plot &plot = GetPlot();
	const Graph *g = GetGraph();
	const Axis   &ax = plot.axis;

	const bool sel = g != NULL;
	const bool vf = g && g->isVectorField();
	const bool color = g && g->isColor();
	const bool line = g && g->isLine();
	const bool histo = g && g->isHistogram();
	const bool twoD = plot.axis_type() == Axis::Rect;
	const bool points = g && g->options.shading_mode == Shading_Points;

	Layout layout(*this, 22);
	const int w1 = 80; // label width
	SET(w1, -1);

	const COLORREF OFF_COLOR = GREY(127);

	if (!header.GetCheck() || plot.axis_type() == Axis::Invalid)
	{
		HIDE(qualityLabel);     HIDE(quality);
		HIDE(discoLabel);       HIDE(disco);
		HIDE(displayModeLabel); HIDE(displayMode); HIDE(vfMode);
		HIDE(histoModeLabel);  HIDE(histoMode);
		HIDE(histoScaleLabel); HIDE(histoScale); HIDE(histoScaleSlider);

		HIDE(gridModeLabel); HIDE(meshModeLabel);
		HIDE(gridMode);      HIDE(meshMode);
		HIDE(gridDensity);   HIDE(meshDensity);

		HIDE(drawAxis);   HIDE(axisModeLabel);
		HIDE(clip);       HIDE(axisMode);
		HIDE(clipCustom); HIDE(clipLock);
		HIDE(clipReset);  HIDE(clipDistance);
	}
	else
	{
		if (vf || (color && g->mode() != GM_RiemannColor))
		{
			HIDE(qualityLabel); HIDE(quality);
		}
		else
		{
			USE(&qualityLabel, &quality);
			qualityLabel.EnableWindow(sel);
			quality.EnableWindow(sel);
			quality.SetPos(sel ? (int)(SLIDER_MAX * g->options.quality) : 0);
		}

		if (vf || color || histo || points)
		{
			HIDE(discoLabel); HIDE(disco);
		}
		else
		{
			USE(&discoLabel, &disco);
			discoLabel.EnableWindow(sel);
			disco.EnableWindow(sel);
			disco.SetCheck(sel && g->options.disco ? BST_CHECKED : BST_UNCHECKED);
		}

		if (line || color)
		{
			HIDE(displayModeLabel); HIDE(displayMode); HIDE(vfMode);
		}
		else if (vf)
		{
			USE(&displayModeLabel, &vfMode);
			HIDE(displayMode);
			displayModeLabel.EnableWindow(true);
			vfMode.EnableWindow(true);
			vfMode.SetCurSel(find(vfMode, g->options.vf_mode));
		}
		else
		{
			USE(&displayModeLabel, &displayMode);
			HIDE(vfMode);
			displayModeLabel.EnableWindow(sel);
			displayMode.EnableWindow(sel);
			displayMode.SetCurSel(sel ? find(displayMode, g->options.shading_mode) : -1);
		}

		if (!g || !histo)
		{
			HIDE(histoModeLabel);  HIDE(histoMode);
			HIDE(histoScaleLabel); HIDE(histoScale); HIDE(histoScaleSlider);
		}
		else
		{
			USE(&histoModeLabel, &histoMode);
			histoMode.SetCurSel(find(histoMode, g->options.hist_mode));

			if (g->options.hist_mode == HM_Riemann)
			{
				HIDE(histoScaleLabel); HIDE(histoScale); HIDE(histoScaleSlider);
			}
			else
			{
				SET(w1, -1, -1);
				USE(&histoScaleLabel, &histoScale, &histoScaleSlider);
				SET(w1, -1);

				// 0 <-> 0, 1/2 <-> 1, 1 <-> M
				// =>  f(x) = (exp(2xln(M-1))-1) / M-2
				// =>       = (2x ^ (M-1) - 1) / M-2
				// =>  f_inv(y) = ln(y * (M-2) + 1) / 2ln(M-1)
				double y = g->options.hist_scale;
				double x = 0.5*log(y*(HISTO_MAX - 2.0) + 1.0) / log(HISTO_MAX - 1.0);
				histoScale.SetDouble(y);
				histoScaleSlider.SetPos(int(SLIDER_MAX * x));
			}
		}

		SET(0, -1, -1, 0);

		if (color || line)
		{
			HIDE(gridModeLabel); HIDE(meshModeLabel);
			HIDE(gridMode);      HIDE(meshMode);
			HIDE(gridDensity);   HIDE(meshDensity);
		}
		else
		{
			layout.skip(2);
			DS0;
			USEH(DS(20), NULL, &gridModeLabel, &meshModeLabel, NULL);
			bool disableMeshLabel = true;
			if (vf || points)
			{
				HIDE(gridMode); HIDE(meshMode);
			}
			else
			{
				USE(NULL, &gridMode, &meshMode, NULL);
				gridMode.EnableWindow(sel);
				gridMode.SetCurSel(sel ? find(gridMode, g->options.grid_style) : -1);
				bool on = g && !histo && g->hasFill();
				meshMode.EnableWindow(on);
				meshMode.SetCurSel(on ? find(meshMode, g->options.mask.style()) : -1);
				if (on) disableMeshLabel = false;
			}
			USE(NULL, &gridDensity, &meshDensity, NULL);
			bool on = g && (!histo || !points);
			gridDensity.EnableWindow(on);
			gridDensity.SetPos(on ? int(SLIDER_MAX * g->options.grid_density) : 0);
			on = g && g->hasFill() && g->options.mask.style() != Mask_Off;
			meshDensity.EnableWindow(on);
			meshDensity.SetPos(on ? int(SLIDER_MAX * g->options.mask.density()) : 0);
			if (on) disableMeshLabel = false;
			meshModeLabel.EnableWindow(!disableMeshLabel);
		}

		USE(NULL, &drawAxis, &axisModeLabel, NULL);
		USE(NULL, &clip, &axisMode, NULL);
		drawAxis.SetCheck(!plot.axis.options.hidden);
		axisModeLabel.EnableWindow(twoD);
		axisMode.EnableWindow(twoD);
		axisMode.SetCurSel(twoD ? find(axisMode, ax.options.axis_grid) : -1);
		clip.EnableWindow(g && !histo && plot.axis_type() == Axis::Box && g->type() != R3_R && !plot.options.clip.on());
		clip.SetCheck(g ? g->clipping() : false);

		if (twoD)
		{
			HIDE(clipCustom); HIDE(clipLock);
			HIDE(clipReset);  HIDE(clipDistance);
		}
		else
		{
			USE(NULL, &clipCustom, &clipLock, NULL);
			USE(NULL, &clipDistance, &clipReset, NULL);

			clipCustom.SetCheck(plot.options.clip.on());
			clipDistance.EnableWindow(plot.options.clip.on());
			clipDistance.SetPos(int(SLIDER_MAX * plot.options.clip.distance()));
			clipReset.EnableWindow(plot.options.clip.on() && plot.options.clip.locked());
			clipLock.EnableWindow(plot.options.clip.on());
			clipLock.SetCheck(plot.options.clip.on() && plot.options.clip.locked());
		}
		layout.skip();
	}

	if (full) MoveWindow(0, 0, layout.W, layout.y);
}

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

	ID_aaModeLabel, ID_aaMode,
	ID_transparencyModeLabel, ID_transparencyMode,
	ID_fogLabel, ID_fog,
	ID_lineWidthLabel, ID_lineWidth,
	ID_shinynessLabel, ID_shinyness,

	ID_bgLabel, ID_fillLabel, ID_axisLabel, ID_gridLabel,
	ID_bgColor, ID_fillColor, ID_axisColor, ID_gridColor,
	ID_bgAlpha, ID_fillAlpha, ID_axisAlpha, ID_gridAlpha,

	ID_textureLabel, ID_reflectionLabel,
	ID_texture, ID_reflection,
	ID_textureStrength, ID_reflectionStrength,
	ID_textureMode,

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
	ON_CBN_SELCHANGE(ID_aaMode, OnAAMode)
	ON_CBN_SELCHANGE(ID_transparencyMode, OnTransparencyMode)
	ON_BN_CLICKED(ID_bgColor, OnBgColor)
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
	else if (sc == &bgAlpha)
	{
		document().plot.axis.options.background_color.SetAlpha(bgAlpha.GetPos());
		Redraw();
	}
	else if (sc == &axisAlpha)
	{
		document().plot.axis.options.axis_color.SetAlpha(axisAlpha.GetPos());
		Redraw();
	}
	else if (sc == &fillAlpha)
	{
		Graph *g = document().plot.current_graph(); if (!g || !g->hasFill()) return;
		g->options.fill_color.SetAlpha(fillAlpha.GetPos());
		Redraw();
	}
	else if (sc == &gridAlpha)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		(g->usesLineColor() ? g->options.line_color : g->options.grid_color).SetAlpha(gridAlpha.GetPos());
		Redraw();
	}
	else if (sc == &fog)
	{
		document().plot.options.fog = fog.GetPos() / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &lineWidth)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		(g->usesLineColor() ? g->options.line_width : g->options.gridline_width) = lineWidth.GetPos() / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &shinyness)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		g->options.shinyness = shinyness.GetPos() / (double)SLIDER_MAX;
		Redraw();
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

void SideSectionSettings::OnVScroll(UINT code, UINT pos, CScrollBar *sb)
{
	CSliderCtrl *sc = (CSliderCtrl*)sb;
	if (sc == &textureStrength)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		g->options.texture_opacity = (SLIDER_MAX - textureStrength.GetPos()) / (double)SLIDER_MAX;
		Redraw();
	}
	else if (sc == &reflectionStrength)
	{
		Graph *g = document().plot.current_graph(); if (!g) return;
		g->options.reflection_opacity = (SLIDER_MAX - reflectionStrength.GetPos()) / (double)SLIDER_MAX;
		Redraw();
	}
	else
	{
		SideSection::OnVScroll(code, pos, sb);
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

void SideSectionSettings::OnAAMode()
{
	Plot &plot = document().plot;

	int i = aaMode.GetCurSel();
	AntialiasMode m = (AntialiasMode)aaMode.GetItemData(i);
	if (m == plot.options.aa_mode) return;

	plot.options.aa_mode = m;
	Redraw();
}

void SideSectionSettings::OnTransparencyMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

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
		Update(false);
		return;
	}

	Redraw();
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

void SideSectionSettings::OnTextureMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = textureMode.GetCurSel();
	TextureProjection m = (TextureProjection)textureMode.GetItemData(i);
	if (m == g->options.texture_projection) return;

	g->options.texture_projection = m;
	Recalc(g);
}

//---------------------------------------------------------------------------------------------
// Textures & Colors
//---------------------------------------------------------------------------------------------

void SideSectionSettings::OnChangeTexture(int i)
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	g->isColor() ? Recalc(g) : Redraw();
}

void SideSectionSettings::OnBgColor()
{
	document().plot.axis.options.background_color = bgColor.GetColor();
	Redraw();

	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().RedrawHeader();
}
void SideSectionSettings::OnAxisColor()
{
	document().plot.axis.options.axis_color = axisColor.GetColor();
	Redraw();
}
void SideSectionSettings::OnFillColor()
{
	Graph *g = document().plot.current_graph(); if (!g || !g->hasFill()) return;
	g->options.fill_color = fillColor.GetColor();
	Redraw();
}
void SideSectionSettings::OnGridColor()
{
	Graph *g = document().plot.current_graph(); if (!g) return;
	(g->usesLineColor() ? g->options.line_color : g->options.grid_color) = gridColor.GetColor();
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

	LABEL(aaModeLabel, "Antialiasing:");
	POPUP(aaMode);
	OPTION("Off");     DATA(AA_Off);
	OPTION("Lines");   DATA(AA_Lines);
	OPTION("4x FSAA"); DATA(AA_4x);
	OPTION("8x FSAA"); DATA(AA_8x);
	/*	MainWindow *w = (MainWindow*)GetParentFrame();
	if (w->GetMainView().GetPlotView().hasAccum)
	{
	OPTION("4x ACC"); DATA(AA_4x_Acc);
	OPTION("8x ACC"); DATA(AA_8x_Acc);
	}*/

	LABEL(transparencyModeLabel, "Transparency:");
	POPUP(transparencyMode);
	for (const GL_DefaultBlendMode &m : DefaultBlendModes())
	{
		transparencyMode.InsertString(currentIdx++, Convert(m.name));
	}
	OPTION("Custom...");

	LABEL(fogLabel, "Fog:");              SLIDER(fog, SLIDER_MAX);
	LABEL(lineWidthLabel, "Line Width:"); SLIDER(lineWidth, 5 * SLIDER_MAX);
	LABEL(shinynessLabel, "Shinyness:");  SLIDER(shinyness, SLIDER_MAX);

	LABEL(bgLabel, "Background:"); COLOR(bgColor); SLIDER(bgAlpha, 255);
	LABEL(fillLabel, "Fill Color:"); COLOR(fillColor); SLIDER(fillAlpha, 255);
	LABEL(axisLabel, "Axis Color:"); COLOR(axisColor); SLIDER(axisAlpha, 255);
	LABEL(gridLabel, "Grid Color:"); COLOR(gridColor); SLIDER(gridAlpha, 255);

	LABEL(textureLabel, "Texture");
	LABEL(reflectionLabel, "Reflection");
	CREATE(texture, 20, WS_CHILD);
	CREATE(reflection, 20, WS_CHILD);
	texture.OnChange = [this]() { OnChangeTexture(0); };
	reflection.OnChange = [this]() { OnChangeTexture(1); };
	VSLIDER(textureStrength, SLIDER_MAX);
	VSLIDER(reflectionStrength, SLIDER_MAX);
	POPUP(textureMode);
	OPTION("Tile");      DATA(TP_Repeat);
	OPTION("Center");    DATA(TP_Center);
	OPTION("Riemann");   DATA(TP_Riemann);
	OPTION("Spherical"); DATA(TP_UV);

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
	const Camera &cam = plot.camera;

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

		HIDE(aaModeLabel);      HIDE(aaMode);
		HIDE(transparencyMode); HIDE(transparencyModeLabel);
		HIDE(fogLabel);         HIDE(fog);
		HIDE(lineWidthLabel);   HIDE(lineWidth);
		HIDE(shinynessLabel);   HIDE(shinyness);

		HIDE(bgLabel); HIDE(bgColor); HIDE(bgAlpha);
		HIDE(fillLabel); HIDE(fillColor); HIDE(fillAlpha);
		HIDE(axisLabel); HIDE(axisColor); HIDE(axisAlpha);
		HIDE(gridLabel); HIDE(gridColor); HIDE(gridAlpha);
		HIDE(textureLabel);
		HIDE(reflectionLabel);
		HIDE(texture);
		HIDE(textureStrength);
		HIDE(reflectionStrength);
		HIDE(reflection);
		HIDE(textureMode);

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

		if (false)
		{
			HIDE(aaModeLabel); HIDE(aaMode);
		}
		else
		{
			USE(&aaModeLabel, &aaMode);
			aaMode.SetCurSel(find(aaMode, plot.options.aa_mode));
		}

		if (vf || line || (g && !color && !g->hasFill()))
		{
			HIDE(transparencyModeLabel); HIDE(transparencyMode);
		}
		else
		{
			USE(&transparencyModeLabel, &transparencyMode);
			const bool on = sel && (color || g->hasFill());
			transparencyModeLabel.EnableWindow(on);
			transparencyMode.EnableWindow(on);
			transparencyMode.SetCurSel(sel ? find(displayMode, g->options.shading_mode) : -1);
			if (!on)
			{
				transparencyMode.SetCurSel(-1);
			}
			else
			{
				auto &tm = DefaultBlendModes();
				int n = (int)tm.size();
				transparencyMode.SetCurSel(n); // custom
				for (int i = 0; i < n; ++i)
				{
					if (g->options.transparency == tm[i].mode)
					{
						transparencyMode.SetCurSel(i);
						break;
					}
				}
			}
		}

		if (twoD)
		{
			HIDE(fogLabel); HIDE(fog);
		}
		else
		{
			USE(&fogLabel, &fog);
			fog.SetPos((int)(SLIDER_MAX * plot.options.fog));
		}

		if (vf || color)
		{
			HIDE(lineWidthLabel); HIDE(lineWidth);
		}
		else
		{
			USE(&lineWidthLabel, &lineWidth);
			const bool on = g && (line || g->options.shading_mode == Shading_Wireframe ||
				g->options.shading_mode == Shading_Points || g->options.grid_style != Grid_Off);
			lineWidthLabel.EnableWindow(on);
			lineWidth.EnableWindow(on);
			lineWidth.SetPos(on ? (int)(SLIDER_MAX * (g->usesLineColor() ? g->options.line_width : g->options.gridline_width)) : 0);
			lineWidthLabel.SetWindowText((g && g->isArea() && g->usesShading() && g->options.shading_mode == Shading_Points) ? _T("Point Size:") : _T("Line Width:"));
		}

		if (vf || color || line || twoD)
		{
			HIDE(shinynessLabel); HIDE(shinyness);
		}
		else
		{
			USE(&shinynessLabel, &shinyness);
			const bool on = g && g->hasNormals();
			shinynessLabel.EnableWindow(on);
			shinyness.EnableWindow(on);
			shinyness.SetPos(on ? (int)(SLIDER_MAX * g->options.shinyness) : 0);
		}

		SET(w1, -1, -1);
		USE(&bgLabel, &bgColor, &bgAlpha);
		bgColor.SetColor(ax.options.background_color);
		bgAlpha.SetPos(ax.options.background_color.GetAlpha());
		
		USE(&fillLabel, &fillColor, &fillAlpha);
		const bool hasFill = g && g->hasFill();
		fillLabel.EnableWindow(hasFill);
		fillColor.EnableWindow(hasFill);
		fillAlpha.EnableWindow(hasFill);
		fillColor.SetColor(hasFill ? (COLORREF)g->options.fill_color : OFF_COLOR);
		fillAlpha.SetPos(hasFill ? g->options.fill_color.GetAlpha() : 0);
		
		USE(&axisLabel, &axisColor, &axisAlpha);
		axisColor.SetColor(ax.options.axis_color);
		axisAlpha.SetPos(ax.options.axis_color.GetAlpha());

		const bool hasGrid = g && !color;
		USE(&gridLabel, &gridColor, &gridAlpha);
		gridLabel.EnableWindow(hasGrid);
		gridColor.EnableWindow(hasGrid);
		gridAlpha.EnableWindow(hasGrid);
		gridLabel.SetWindowText(g && g->usesLineColor() ? _T("Line Color:") : _T("Grid Color"));
		gridColor.SetColor(hasGrid ? (COLORREF)(g->usesLineColor() ? g->options.line_color : g->options.grid_color) : OFF_COLOR);
		gridAlpha.SetPos(hasGrid ? (g->usesLineColor() ? g->options.line_color : g->options.grid_color).GetAlpha() : 0);

		if (vf || line || points)
		{
			HIDE(textureLabel);
			HIDE(reflectionLabel);
			HIDE(texture);
			HIDE(textureStrength);
			HIDE(reflectionStrength);
			HIDE(reflection);
			HIDE(textureMode);
		}
		else
		{
			const bool hasTex = g && (color || g->hasFill());
			const bool hasRef = g && !color && !twoD && g->hasNormals();

			layout.skip(2);
			SET(0, -1, 20, 20, -1, 0);
			USE(NULL, &textureLabel, NULL, NULL, &reflectionLabel, NULL);
			textureLabel.EnableWindow(hasTex);
			reflectionLabel.EnableWindow(hasRef);

			USEH(layout[1], NULL, &texture, &textureStrength, &reflectionStrength, &reflection, NULL);
			texture.EnableWindow(hasTex);
			textureStrength.EnableWindow(hasTex);
			reflection.EnableWindow(hasRef);
			reflectionStrength.EnableWindow(hasRef);
			textureStrength.SetPos(color ? 0 : SLIDER_MAX - (hasTex ? (int)(g->options.texture_opacity*SLIDER_MAX) : 0));
			reflectionStrength.SetPos(SLIDER_MAX - (hasRef ? (int)(g->options.reflection_opacity*SLIDER_MAX) : 0));
			texture.SetImage(hasTex ? const_cast<GL_Image*>(&g->options.texture) : NULL, full);
			reflection.SetImage(hasRef ? const_cast<GL_Image*>(&g->options.reflection_texture) : NULL, full);

			if (!color)
			{
				HIDE(textureMode);
			}
			else
			{
				USE(NULL, &textureMode, NULL, NULL, NULL, NULL);
				textureMode.SetCurSel(sel ? find(textureMode, g->options.texture_projection) : -1);
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
			layout.skip(4);
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

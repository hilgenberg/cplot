#include "../stdafx.h"
#include "SideSectionStyle.h"
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

	ID_aaModeLabel, ID_aaMode,
	ID_transparencyModeLabel, ID_transparencyMode,
	ID_fogLabel, ID_fog,
	ID_lineWidthLabel, ID_lineWidth,
	ID_shinynessLabel, ID_shinyness,

	ID_bgLabel, ID_fillLabel, ID_axisLabel, ID_gridLabel,
	ID_bgColor, ID_fillColor, ID_axisColor, ID_gridColor,
	ID_bgAlpha, ID_fillAlpha, ID_axisAlpha, ID_gridAlpha,
	ID_fontLabel, ID_font,

	ID_textureLabel, ID_reflectionLabel,
	ID_texture, ID_reflection,
	ID_textureStrength, ID_reflectionStrength,
	ID_textureMode,
};

BEGIN_MESSAGE_MAP(SideSectionStyle, SideSection)
	ON_WM_CREATE()
	ON_CBN_SELCHANGE(ID_aaMode, OnAAMode)
	ON_CBN_SELCHANGE(ID_transparencyMode, OnTransparencyMode)
	ON_BN_CLICKED(ID_bgColor, OnBgColor)
	ON_BN_CLICKED(ID_fillColor, OnFillColor)
	ON_BN_CLICKED(ID_axisColor, OnAxisColor)
	ON_BN_CLICKED(ID_gridColor, OnGridColor)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(ID_textureMode, OnTextureMode)
	ON_BN_CLICKED(ID_font, OnFont)
END_MESSAGE_MAP()

static constexpr int    SLIDER_MAX = 64000;
static constexpr double HISTO_MAX  = 1.0e4;

//---------------------------------------------------------------------------------------------
// Check Boxes & Buttons
//---------------------------------------------------------------------------------------------

void SideSectionStyle::OnFont()
{
	GL_Font &f = document().plot.axis.options.label_font;

	LOGFONT lf; memset(&lf, 0, sizeof(LOGFONT));
	CClientDC dc(this);
	lf.lfHeight = -(LONG)(f.size * dc.GetDeviceCaps(LOGPIXELSY) / 72.0f);
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, Convert(f.name));

	CFontDialog dlg(&lf);
	if (dlg.DoModal() != IDOK) return;

	f.name = Convert(dlg.GetFaceName());
	f.size = 0.1f * (float)dlg.GetSize();

	Update(false);
	Redraw();
}
//---------------------------------------------------------------------------------------------
// Sliders
//---------------------------------------------------------------------------------------------

void SideSectionStyle::OnHScroll(UINT code, UINT pos, CScrollBar *sb)
{
	CSliderCtrl *sc = (CSliderCtrl*)sb;
	if (sc == &bgAlpha)
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
	else
	{
		SideSection::OnHScroll(code, pos, sb);
	}
}

void SideSectionStyle::OnVScroll(UINT code, UINT pos, CScrollBar *sb)
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
// Combo Boxes
//---------------------------------------------------------------------------------------------

void SideSectionStyle::OnAAMode()
{
	Plot &plot = document().plot;

	int i = aaMode.GetCurSel();
	AntialiasMode m = (AntialiasMode)aaMode.GetItemData(i);
	if (m == plot.options.aa_mode) return;

	plot.options.aa_mode = m;
	Redraw();
}

void SideSectionStyle::OnTransparencyMode()
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

void SideSectionStyle::OnTextureMode()
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	int i = textureMode.GetCurSel();
	TextureProjection m = (TextureProjection)textureMode.GetItemData(i);
	if (m == g->options.texture_projection) return;

	g->options.texture_projection = m;
	Recalc(g);
}

void SideSectionStyle::OnCycleTextureMode(int d)
{
	assert(d == 1 || d == -1);
	Graph *g = document().plot.current_graph(); if (!g) return;

	int m = d + (int)g->options.texture_projection;
	if (m < 0) m = TP_LAST;
	if (m > TP_LAST) m = 0;

	g->options.texture_projection = (TextureProjection)m;
	Recalc(g);
	Update(false);
}

//---------------------------------------------------------------------------------------------
// Textures & Colors
//---------------------------------------------------------------------------------------------

void SideSectionStyle::OnChangeTexture(int i)
{
	Graph *g = document().plot.current_graph(); if (!g) return;

	g->isColor() ? Recalc(g) : Redraw();
}

void SideSectionStyle::OnBgColor()
{
	document().plot.axis.options.background_color = bgColor.GetColor();
	Redraw();

	MainWindow *w = (MainWindow*)GetParentFrame();
	w->GetMainView().RedrawHeader();
}
void SideSectionStyle::OnAxisColor()
{
	document().plot.axis.options.axis_color = axisColor.GetColor();
	Redraw();
}
void SideSectionStyle::OnFillColor()
{
	Graph *g = document().plot.current_graph(); if (!g || !g->hasFill()) return;
	g->options.fill_color = fillColor.GetColor();
	Redraw();
}
void SideSectionStyle::OnGridColor()
{
	Graph *g = document().plot.current_graph(); if (!g) return;
	(g->usesLineColor() ? g->options.line_color : g->options.grid_color) = gridColor.GetColor();
	Redraw();
}

//---------------------------------------------------------------------------------------------
// Create & Update
//---------------------------------------------------------------------------------------------

int SideSectionStyle::OnCreate(LPCREATESTRUCT cs)
{
	if (SideSection::OnCreate(cs) < 0) return -1;

	START_CREATE;

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
	LABEL(fontLabel, "Axis Font:");
	BUTTON(font, "Arial 12");

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

void SideSectionStyle::Update(bool full)
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
		HIDE(aaModeLabel);      HIDE(aaMode);
		HIDE(transparencyMode); HIDE(transparencyModeLabel);
		HIDE(fogLabel);         HIDE(fog);
		HIDE(lineWidthLabel);   HIDE(lineWidth);
		HIDE(shinynessLabel);   HIDE(shinyness);

		HIDE(bgLabel); HIDE(bgColor); HIDE(bgAlpha);
		HIDE(fillLabel); HIDE(fillColor); HIDE(fillAlpha);
		HIDE(axisLabel); HIDE(axisColor); HIDE(axisAlpha);
		HIDE(gridLabel); HIDE(gridColor); HIDE(gridAlpha);
		HIDE(fontLabel); HIDE(font);
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
		USE(&aaModeLabel, &aaMode);
		aaMode.SetCurSel(find(aaMode, plot.options.aa_mode));

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
			transparencyMode.SetCurSel(sel ? find(transparencyMode, g->options.shading_mode) : -1);
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

		USE(&fontLabel, &font, &font);
		font.SetWindowText(Convert(plot.axis.options.label_font.to_string()));

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
			texture.SetImage(hasTex ? const_cast<GL_Image*>(&g->options.texture) : NULL);
			reflection.SetImage(hasRef ? const_cast<GL_Image*>(&g->options.reflection_texture) : NULL);

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

		layout.skip();
	}

	if (full) MoveWindow(0, 0, layout.W, layout.y);
}

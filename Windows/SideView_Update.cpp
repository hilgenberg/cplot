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
#include "../Engine/Namespace/UserFunction.h"

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
	CREATE(texture, WS_CHILD);
	CREATE(reflection, WS_CHILD);
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

	//----------------------------------------------------------------------------------

	SECTION(axis, "Axis");
	LABEL(centerLabel, "Center"); LABEL(rangeLabel, "Range");
	LABEL(xLabel, "x:"); EDIT(xCenter); EDIT(xRange); DELTA(xDelta);
	LABEL(yLabel, "y:"); EDIT(yCenter); EDIT(yRange); DELTA(yDelta);
	LABEL(zLabel, "z:"); EDIT(zCenter); EDIT(zRange); DELTA(zDelta);
	DELTA(xyzDelta);
	LABEL(uLabel, "u:"); EDIT(uCenter); EDIT(uRange); DELTA(uDelta);
	LABEL(vLabel, "v:"); EDIT(vCenter); EDIT(vRange); DELTA(vDelta);
	DELTA(uvDelta);
	LABEL(phiLabel, "phi"); LABEL(psiLabel, "psi"); LABEL(thetaLabel, "theta");
	EDIT(phi); EDIT(psi); EDIT(theta);
	DELTA(phiDelta); DELTA(psiDelta); DELTA(thetaDelta);
	LABEL(distLabel, "Zoom:"); EDIT(dist); DELTA(distDelta);
	BUTTON(center, "Center"); BUTTON(top, "Upright"); BUTTON(front, "Front");

	xCenter.OnChange = [this] { OnAxisCenter( 0, xCenter); };
	yCenter.OnChange = [this] { OnAxisCenter( 1, yCenter); };
	zCenter.OnChange = [this] { OnAxisCenter( 2, zCenter); };
	uCenter.OnChange = [this] { OnAxisCenter(-1, uCenter); };
	vCenter.OnChange = [this] { OnAxisCenter(-2, vCenter); };

	xRange.OnChange = [this] { OnAxisRange( 0, xRange); };
	yRange.OnChange = [this] { OnAxisRange( 1, yRange); };
	zRange.OnChange = [this] { OnAxisRange( 2, zRange); };
	uRange.OnChange = [this] { OnAxisRange(-1, uRange); };
	vRange.OnChange = [this] { OnAxisRange(-2, vRange); };

	phi.OnChange   = [this] { OnAxisAngle(0, phi); };
	psi.OnChange   = [this] { OnAxisAngle(1, psi); };
	theta.OnChange = [this] { OnAxisAngle(2, theta); };

	dist.OnChange = [this] { OnDistance(); };

	return 0;
}

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
	if (!active_anims || !doc) return;
	Plot &plot = doc->plot;
	Axis &axis = plot.axis;
	Camera &camera = plot.camera;

	double t = now();
	bool recalc = false;

	double dx = xDelta.evolve(t);
	double dy = yDelta.evolve(t);
	double dz = zDelta.evolve(t);
	double dr = xyzDelta.evolve(t);
	if (dx || dy || dz)
	{
		double scale = 0.05;
		dx *= axis.range(0) * scale;
		dy *= axis.range(1) * scale;
		dz *= axis.range(2) * scale;
		axis.move(dx, dy, dz);
		recalc = true;
	}
	if (dr)
	{
		axis.zoom(1.0 + 0.05 * dr);
		recalc = true;
	}

	double du = uDelta.evolve(t);
	double dv = vDelta.evolve(t);
	double duv = uvDelta.evolve(t);
	if (du || dv)
	{
		double scale = 0.05;
		du *= axis.in_range(0) * scale;
		dv *= axis.in_range(1) * scale;
		axis.in_move(du, dv);
		recalc = true;
	}
	if (duv)
	{
		axis.in_zoom(1.0 + 0.05 * duv);
		recalc = true;
	}

	double dphi = phiDelta.evolve(t);
	double dpsi = psiDelta.evolve(t);
	double dtheta = thetaDelta.evolve(t);
	if (dphi) camera.set_phi(camera.phi() + dphi * 10.0);
	if (dpsi) camera.set_psi(camera.psi() + dpsi * 10.0);
	if (dtheta) camera.set_theta(camera.theta() + dtheta * 10.0);

	double dd = distDelta.evolve(t);
	if (dd) camera.zoom(1.0 + 0.05 * dd);

	for (auto *q : params) q->Animate(t);

	if (recalc) Recalc(plot);
	UpdateAxis();
	UpdateWindow();
}

static int find(CComboBox &b, DWORD_PTR itemData)
{
	for (int i = 0, n = b.GetCount(); i < n; ++i)
	{
		if (b.GetItemData(i) == itemData) return i;
	}
	return -1;
}

void SideView::Update()
{
	CRect bounds; GetClientRect(bounds);
	if (!doc || bounds.Width() < 2) return;

	const Plot   &plot = doc->plot;
	const Graph  *g = plot.current_graph();
	const Axis   &ax = plot.axis;
	const Camera &cam = plot.camera;
	DS0;

	const bool sel = g != NULL;
	const bool vf = g && g->isVectorField();
	const bool color = g && g->isColor();
	const bool line = g && g->isLine();
	const bool histo = g && g->isHistogram();
	const bool twoD = plot.axis_type() == Axis::Rect;
	const bool points = g && g->options.shading_mode == Shading_Points;

	const int W = bounds.Width();
	const int SPC = DS(5); // amount of spacing
	const int y0 = -GetScrollPos(SB_VERT);
	int       y = y0;// y for next control
	const int x0 = SPC;  // row x start / amount of space on the left

	const int h_section = DS(20), h_label = DS(14), h_combo = DS(21), h_check = DS(20),
		h_edit = DS(20), h_color = DS(20), h_slider = DS(20), h_delta = h_slider,
		w_slider = h_slider, h_button = h_check, h_row = DS(24);

	const COLORREF OFF_COLOR = GREY(127);

	//----------------------------------------------------------------------------------
	MOVE(parameters, 0, W, y, h_section, h_section); y += h_row - (h_row - h_section) / 2;
	if (!parameters.GetCheck())
	{
		for (auto *p : params) HIDE(*p);
	}
	else
	{
		// add new parameters, remove deleted ones, sort by name
		std::map<Parameter*, ParameterView*, std::function<bool(const Parameter*, const Parameter*)>>
			m([](const Parameter *a, const Parameter *b) { return a->name() < b->name(); });
		for (auto *q : params)
		{
			auto *p = q->parameter();
			if (!p)
			{
				delete q;
			}
			else
			{
				m.insert(std::make_pair(p, q));
			}
		}
		for (Parameter *p : plot.ns.all_parameters(true))
		{
			if (m.count(p)) continue;
			ParameterView *q = new ParameterView(*this, *p);
			m.insert(std::make_pair(p, q));
			q->Create(CRect(0, 0, 20, 20), this, 2000 + (UINT)p->oid());
		}
		params.clear(); params.reserve(m.size());
		for (auto i : m) params.push_back(i.second);
		
		// place the controls
		for (auto *q : params)
		{
			int hq = q->height(W);
			MOVE(*q, 0, W, y, hq, hq);
			q->Update();
			y += hq;
		}
		if (!params.empty()) y += SPC;
	}
	//----------------------------------------------------------------------------------
	MOVE(definitions, 0, W, y, h_section, h_row); y += h_row;
	if (!definitions.GetCheck())
	{
		for (auto *d : defs) HIDE(*d);
	}
	else
	{
		// add new parameters, remove deleted ones, sort by name
		std::map<UserFunction*, DefinitionView*, std::function<bool(const UserFunction*, const UserFunction*)>>
			m([](const UserFunction *a, const UserFunction *b) { return a->formula() < b->formula(); });
		for (auto *q : defs)
		{
			auto *f = q->function();
			if (!f)
			{
				delete q;
			}
			else
			{
				m.insert(std::make_pair(f, q));
			}
		}
		for (UserFunction *f : plot.ns.all_functions(true))
		{
			if (m.count(f)) continue;
			DefinitionView *q = new DefinitionView(*this, *f);
			m.insert(std::make_pair(f, q));
			q->Create(CRect(0, 0, 20, 20), this, 2000 + (UINT)f->oid());
		}
		defs.clear(); defs.reserve(m.size());
		for (auto i : m) defs.push_back(i.second);

		// place the controls
		for (auto *q : defs)
		{
			int hq = q->height(W);
			MOVE(*q, 0, W, y, hq, hq);
			q->Update();
			y += hq;
		}
		if (!defs.empty()) y += SPC;
	}
	//----------------------------------------------------------------------------------
	MOVE(graphs, 0, W, y, h_section, h_row); y += h_row;
	graphs.SetCanRemove(g ? HeaderControl::Active : HeaderControl::Inactive);
	if (!graphs.GetCheck())
	{
		for (auto *d : gdefs) HIDE(*d);
	}
	else
	{
		// add new graphs, remove deleted ones, sort by name
		std::map<Graph*, GraphView*> m;
		for (auto *q : gdefs)
		{
			auto *f = q->graph();
			if (!f)
			{
				delete q;
			}
			else
			{
				m.insert(std::make_pair(f, q));
			}
		}
		for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
		{
			Graph *f = plot.graph(i);
			if (m.count(f)) continue;
			GraphView *q = new GraphView(*this, *f);
			m.insert(std::make_pair(f, q));
			q->Create(CRect(0, 0, 20, 20), this, 2000 + (UINT)f->oid());
		}
		gdefs.clear(); gdefs.reserve(m.size());
		for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
		{
			Graph *f = plot.graph(i);
			auto it = m.find(f);
			if (it == m.end()) { assert(false); continue; }
			gdefs.push_back(it->second);
		}

		// place the controls
		for (auto *q : gdefs)
		{
			int hq = q->height(W);
			MOVE(*q, 0, W, y, hq, hq);
			q->Update();
			y += hq;
		}
		if (!gdefs.empty()) y += SPC;
	}
	//----------------------------------------------------------------------------------
	MOVE(settings, 0, W, y, h_section, h_row); y += h_row;
	if (!settings.GetCheck())
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
		const int w1 = DS(80); // label width
		const int x1 = W - SPC;
		const int xm = x0 + w1, xmm = xm + SPC;

		if (vf || (color && g->mode() != GM_RiemannColor))
		{
			HIDE(qualityLabel); HIDE(quality);
		}
		else
		{
			MOVE(qualityLabel, x0, xm, y, h_label, h_row);
			MOVE(quality, xmm, x1, y, h_slider, h_row);
			qualityLabel.EnableWindow(sel);
			quality.EnableWindow(sel);
			quality.SetPos(sel ? (int)(SLIDER_MAX * g->options.quality) : 0);
			y += h_row;
		}

		if (vf || color || histo || points)
		{
			HIDE(discoLabel); HIDE(disco);
		}
		else
		{
			MOVE(discoLabel, x0, xm, y, h_label, h_row);
			MOVE(disco, xmm, x1, y, h_check, h_row);
			discoLabel.EnableWindow(sel);
			disco.EnableWindow(sel);
			disco.SetCheck(sel && g->options.disco ? BST_CHECKED : BST_UNCHECKED);
			y += h_row;
		}

		if (line || color)
		{
			HIDE(displayModeLabel); HIDE(displayMode); HIDE(vfMode);
		}
		else if (vf)
		{
			HIDE(displayMode);
			MOVE(displayModeLabel, x0, xm, y, h_label, h_row);
			MOVE(vfMode, xmm, x1, y, h_combo, h_row);
			displayModeLabel.EnableWindow(true);
			vfMode.EnableWindow(true);
			vfMode.SetCurSel(find(vfMode, g->options.vf_mode));
			y += h_row;
		}
		else
		{
			HIDE(vfMode);
			MOVE(displayModeLabel, x0, xm, y, h_label, h_row);
			MOVE(displayMode, xmm, x1, y, h_combo, h_row);
			displayModeLabel.EnableWindow(sel);
			displayMode.EnableWindow(sel);
			displayMode.SetCurSel(sel ? find(displayMode, g->options.shading_mode) : -1);
			y += h_row;
		}

		if (!g || !histo)
		{
			HIDE(histoModeLabel);  HIDE(histoMode);
			HIDE(histoScaleLabel); HIDE(histoScale); HIDE(histoScaleSlider);
		}
		else
		{
			MOVE(histoModeLabel, x0, xm, y, h_label, h_row);
			MOVE(histoMode, xmm, x1, y, h_combo, h_row);
			y += h_row;
			histoMode.SetCurSel(find(histoMode, g->options.hist_mode));

			if (g->options.hist_mode == HM_Riemann)
			{
				HIDE(histoScaleLabel); HIDE(histoScale); HIDE(histoScaleSlider);
			}
			else
			{
				MOVE(histoScaleLabel, x0, xm, y, h_label, h_row);
				int d = (x1 - xmm - SPC) / 2;
				MOVE(histoScale, xmm, xmm + d, y, h_edit, h_row);
				MOVE(histoScaleSlider, xmm + d + SPC, x1, y, h_slider, h_row);
				y += h_row;

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
			MOVE(aaModeLabel, x0, xm, y, h_label, h_row);
			MOVE(aaMode, xmm, x1, y, h_combo, h_row);
			aaMode.SetCurSel(find(aaMode, plot.options.aa_mode));
			y += h_row;
		}

		if (vf || line || (g && !color && !g->hasFill()))
		{
			HIDE(transparencyModeLabel); HIDE(transparencyMode);
		}
		else
		{
			MOVE(transparencyModeLabel, x0, xm, y, h_label, h_row);
			MOVE(transparencyMode, xmm, x1, y, h_combo, h_row);
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
			y += h_row;
		}

		if (twoD)
		{
			HIDE(fogLabel); HIDE(fog);
		}
		else
		{
			MOVE(fogLabel, x0, xm, y, h_label, h_row);
			MOVE(fog, xmm, x1, y, h_slider, h_row);
			fog.SetPos((int)(SLIDER_MAX * plot.options.fog));
			y += h_row;
		}

		if (vf || color)
		{
			HIDE(lineWidthLabel); HIDE(lineWidth);
		}
		else
		{
			MOVE(lineWidthLabel, x0, xm, y, h_label, h_row);
			MOVE(lineWidth, xmm, x1, y, h_slider, h_row);
			const bool on = g && (line || g->options.shading_mode == Shading_Wireframe ||
				g->options.shading_mode == Shading_Points || g->options.grid_style != Grid_Off);
			lineWidthLabel.EnableWindow(on);
			lineWidth.EnableWindow(on);
			lineWidth.SetPos(on ? (int)(SLIDER_MAX * (g->usesLineColor() ? g->options.line_width : g->options.gridline_width)) : 0);
			lineWidthLabel.SetWindowText((g && g->isArea() && g->usesShading() && g->options.shading_mode == Shading_Points) ? _T("Point Size:") : _T("Line Width:"));
			y += h_row;
		}

		if (vf || color || line || twoD)
		{
			HIDE(shinynessLabel); HIDE(shinyness);
		}
		else
		{
			MOVE(shinynessLabel, x0, xm, y, h_label, h_row);
			MOVE(shinyness, xmm, x1, y, h_slider, h_row);
			const bool on = g && g->hasNormals();
			shinynessLabel.EnableWindow(on);
			shinyness.EnableWindow(on);
			shinyness.SetPos(on ? (int)(SLIDER_MAX * g->options.shinyness) : 0);
			y += h_row;
		}

		int d = (W - w1 - 4 * SPC) / 2;
		MOVE(bgLabel, x0, xm, y, h_label, h_row);
		MOVE(bgColor, xmm, xmm + d, y, h_color, h_row);
		MOVE(bgAlpha, xmm + d + SPC, x1, y, h_color, h_row);
		bgColor.SetColor(ax.options.background_color);
		bgAlpha.SetPos(ax.options.background_color.GetAlpha());
		y += h_row;
		MOVE(fillLabel, x0, xm, y, h_label, h_row);
		MOVE(fillColor, xmm, xmm + d, y, h_color, h_row);
		MOVE(fillAlpha, xmm + d + SPC, x1, y, h_color, h_row);
		const bool hasFill = g && g->hasFill();
		fillLabel.EnableWindow(hasFill);
		fillColor.EnableWindow(hasFill);
		fillAlpha.EnableWindow(hasFill);
		fillColor.SetColor(hasFill ? (COLORREF)g->options.fill_color : OFF_COLOR);
		fillAlpha.SetPos(hasFill ? g->options.fill_color.GetAlpha() : 0);
		y += h_row;
		MOVE(axisLabel, x0, xm, y, h_label, h_row);
		MOVE(axisColor, xmm, xmm + d, y, h_color, h_row);
		MOVE(axisAlpha, xmm + d + SPC, x1, y, h_color, h_row);
		axisColor.SetColor(ax.options.axis_color);
		axisAlpha.SetPos(ax.options.axis_color.GetAlpha());
		y += h_row;
		const bool hasGrid = g && !color;
		MOVE(gridLabel, x0, xm, y, h_label, h_row);
		MOVE(gridColor, xmm, xmm + d, y, h_color, h_row);
		MOVE(gridAlpha, xmm + d + SPC, x1, y, h_color, h_row);
		gridLabel.EnableWindow(hasGrid);
		gridColor.EnableWindow(hasGrid);
		gridAlpha.EnableWindow(hasGrid);
		gridLabel.SetWindowText(g && g->usesLineColor() ? _T("Line Color:") : _T("Grid Color"));
		gridColor.SetColor(hasGrid ? (COLORREF)(g->usesLineColor() ? g->options.line_color : g->options.grid_color) : OFF_COLOR);
		gridAlpha.SetPos(hasGrid ? (g->usesLineColor() ? g->options.line_color : g->options.grid_color).GetAlpha() : 0);
		y += h_row;

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
			const int dt = (W - 9 * SPC - 2 * w_slider) / 2;
			const int t1 = x0 + 2 * SPC, t2 = t1 + dt, t3 = W - 3 * SPC - dt, t4 = t3 + dt;

			MOVE(textureLabel, t1, t2, y, h_label, h_row);
			MOVE(reflectionLabel, t3, t4, y, h_label, h_row);
			gridLabel.EnableWindow(hasTex);
			reflectionLabel.EnableWindow(hasRef);
			y += h_row;

			MOVE(texture, t1, t2, y, dt, dt);
			MOVE(textureStrength, t2 + SPC, t2 + SPC + w_slider, y, dt, dt);
			MOVE(reflectionStrength, t3 - SPC - w_slider, t3 - SPC, y, dt, dt);
			MOVE(reflection, t3, t4, y, dt, dt);
			texture.EnableWindow(hasTex);
			textureStrength.EnableWindow(hasTex);
			reflection.EnableWindow(hasRef);
			reflectionStrength.EnableWindow(hasRef);
			textureStrength.SetPos(color ? 0 : SLIDER_MAX - (hasTex ? (int)(g->options.texture_opacity*SLIDER_MAX) : 0));
			reflectionStrength.SetPos(SLIDER_MAX - (hasRef ? (int)(g->options.reflection_opacity*SLIDER_MAX) : 0));
			texture.SetImage(hasTex ? const_cast<GL_Image*>(&g->options.texture) : NULL);
			reflection.SetImage(hasRef ? const_cast<GL_Image*>(&g->options.reflection_texture) : NULL);
			y += dt;

			if (!color)
			{
				HIDE(textureMode);
			}
			else
			{
				MOVE(textureMode, t1, t2, y, h_combo, h_row);
				textureMode.SetCurSel(sel ? find(textureMode, g->options.texture_projection) : -1);
				y += h_row;
			}
		}

		const int dt = (W - 7 * SPC) / 2;
		const int t1 = x0 + 2 * SPC, t2 = t1 + dt, t3 = t2 + SPC, t4 = t3 + dt;

		if (color || line)
		{
			HIDE(gridModeLabel); HIDE(meshModeLabel);
			HIDE(gridMode);      HIDE(meshMode);
			HIDE(gridDensity);   HIDE(meshDensity);
		}
		else
		{
			y += 4 * SPC;
			MOVE(gridModeLabel, t1, t2, y, h_label, h_label);
			MOVE(meshModeLabel, t3, t4, y, h_label, h_label);
			bool disableMeshLabel = true;
			y += h_label;
			if (vf || points)
			{
				HIDE(gridMode); HIDE(meshMode);
			}
			else
			{
				y += SPC;
				MOVE(gridMode, t1, t2, y, h_combo, h_row);
				MOVE(meshMode, t3, t4, y, h_combo, h_row);
				gridMode.EnableWindow(sel);
				gridMode.SetCurSel(sel ? find(gridMode, g->options.grid_style) : -1);
				bool on = g && !histo && g->hasFill();
				meshMode.EnableWindow(on);
				meshMode.SetCurSel(on ? find(meshMode, g->options.mask.style()) : -1);
				if (on) disableMeshLabel = false;
				y += h_row;
			}
			MOVE(gridDensity, t1, t2, y, h_combo, h_row);
			MOVE(meshDensity, t3, t4, y, h_combo, h_row);
			bool on = g && (!histo || !points);
			gridDensity.EnableWindow(on);
			gridDensity.SetPos(on ? int(SLIDER_MAX * g->options.grid_density) : 0);
			on = g && g->hasFill() && g->options.mask.style() != Mask_Off;
			meshDensity.EnableWindow(on);
			meshDensity.SetPos(on ? int(SLIDER_MAX * g->options.mask.density()) : 0);
			if (on) disableMeshLabel = false;
			meshModeLabel.EnableWindow(!disableMeshLabel);
			y += h_row;
		}

		MOVE(drawAxis, t1, t2, y, h_check, h_row);
		MOVE(axisModeLabel, t3, t4, y, h_label, h_row);
		y += h_row;
		MOVE(clip, t1, t2, y, h_check, h_row);
		MOVE(axisMode, t3, t4, y, h_combo, h_row);
		drawAxis.SetCheck(!plot.axis.options.hidden);
		axisModeLabel.EnableWindow(twoD);
		axisMode.EnableWindow(twoD);
		axisMode.SetCurSel(twoD ? find(axisMode, ax.options.axis_grid) : -1);
		clip.EnableWindow(g && !histo && plot.axis_type() == Axis::Box && g->type() != R3_R && !plot.options.clip.on());
		clip.SetCheck(g ? g->clipping() : false);
		y += h_row;

		if (twoD)
		{
			HIDE(clipCustom); HIDE(clipLock);
			HIDE(clipReset);  HIDE(clipDistance);
		}
		else
		{
			MOVE(clipCustom, t1, t2, y, h_check, h_row);
			MOVE(clipLock, t3, t4, y, h_check, h_row);
			y += h_row;
			MOVE(clipDistance, t1, t2, y, h_slider, h_row);
			MOVE(clipReset, t3, t4, y, h_button, h_row);
			y += h_row;

			clipCustom.SetCheck(plot.options.clip.on());
			clipDistance.EnableWindow(plot.options.clip.on());
			clipDistance.SetPos(int(SLIDER_MAX * plot.options.clip.distance()));
			clipReset.EnableWindow(plot.options.clip.on() && plot.options.clip.locked());
			clipLock.EnableWindow(plot.options.clip.on());
			clipLock.SetCheck(plot.options.clip.on() && plot.options.clip.locked());
		}
		y += SPC;
	}
	//----------------------------------------------------------------------------------
	MOVE(axis, 0, W, y, h_section, h_row); y += h_row;
	if (!axis.GetCheck())
	{
		HIDE(centerLabel); HIDE(rangeLabel);
		HIDE(xLabel); HIDE(xCenter); HIDE(xRange); HIDE(xDelta);
		HIDE(yLabel); HIDE(yCenter); HIDE(yRange); HIDE(yDelta);
		HIDE(zLabel); HIDE(zCenter); HIDE(zRange); HIDE(zDelta);
		HIDE(xyzDelta);
		HIDE(uLabel); HIDE(uCenter); HIDE(uRange); HIDE(uDelta);
		HIDE(vLabel); HIDE(vCenter); HIDE(vRange); HIDE(vDelta);
		HIDE(uvDelta);
		HIDE(phiLabel); HIDE(psiLabel); HIDE(thetaLabel);
		HIDE(phi); HIDE(psi); HIDE(theta);
		HIDE(phiDelta); HIDE(psiDelta); HIDE(thetaDelta);
		HIDE(distLabel); HIDE(dist); HIDE(distDelta);
		HIDE(center); HIDE(top); HIDE(front);
	}
	else
	{
		const int w1 = DS(20); // label width
		int d = (W - 2 * (w1 + 2 * SPC) - 2 * SPC) / 3;
		const int x1 = x0 + w1 + SPC, x2 = x1 + d + SPC, x3 = x2 + d + SPC, x4 = x3 + d + SPC, xe = W - SPC;
		const bool in3d = (plot.axis_type() != Axis::Rect);

		MOVE(centerLabel, x1, x2 - SPC, y, h_label, h_row);
		MOVE(rangeLabel, x2, x3 - SPC, y, h_label, h_row);
		y += h_row - DS(5);

		MOVE(xLabel, x0, x1 - SPC, y, h_label, h_row);
		MOVE(xCenter, x1, x2 - SPC, y, h_edit, h_row);
		MOVE(xRange, x2, x3 - SPC, y, h_edit, h_row);
		MOVE(xDelta, x3, xe, y, h_delta, h_row);
		y += h_row;
		xCenter.SetDouble(ax.center(0));
		xRange.SetDouble(ax.range(0)*2.0);

		MOVE(yLabel, x0, x1 - SPC, y, h_label, h_row);
		MOVE(yCenter, x1, x2 - SPC, y, h_edit, h_row);
		MOVE(yRange, x2, x3 - SPC, y, h_edit, h_row);
		MOVE(yDelta, x3, xe, y, h_delta, h_row);
		y += h_row;
		yCenter.SetDouble(ax.center(1));
		yRange.SetDouble(ax.range(1)*2.0);
		yRange.EnableWindow(in3d);

		if (!in3d)
		{
			HIDE(zLabel);
			HIDE(zCenter);
			HIDE(zRange);
			HIDE(zDelta);
		}
		else
		{
			MOVE(zLabel, x0, x1 - SPC, y, h_label, h_row);
			MOVE(zCenter, x1, x2 - SPC, y, h_edit, h_row);
			MOVE(zRange, x2, x3 - SPC, y, h_edit, h_row);
			MOVE(zDelta, x3, xe, y, h_delta, h_row);
			y += h_row;
			zCenter.SetDouble(ax.center(2));
			zRange.SetDouble(ax.range(2)*2.0);
		}
		MOVE(xyzDelta, x1, x3 - SPC, y, h_delta, h_row);
		y += h_row;

		const int nin = g ? g->inRangeDimension() : -1;

		if (nin < 1)
		{
			HIDE(uLabel);
			HIDE(uCenter);
			HIDE(uRange);
			HIDE(uDelta);
		}
		else
		{
			MOVE(uLabel, x0, x1 - SPC, y, h_label, h_row);
			MOVE(uCenter, x1, x2 - SPC, y, h_edit, h_row);
			MOVE(uRange, x2, x3 - SPC, y, h_edit, h_row);
			MOVE(uDelta, x3, xe, y, h_delta, h_row);
			y += h_row;
			uCenter.SetDouble(ax.in_center(0));
			uRange.SetDouble(ax.in_range(0)*2.0);
		}
		if (nin < 2)
		{
			HIDE(vLabel);
			HIDE(vCenter);
			HIDE(vRange);
			HIDE(vDelta);
		}
		else
		{
			MOVE(vLabel, x0, x1 - SPC, y, h_label, h_row);
			MOVE(vCenter, x1, x2 - SPC, y, h_edit, h_row);
			MOVE(vRange, x2, x3 - SPC, y, h_edit, h_row);
			MOVE(vDelta, x3, xe, y, h_delta, h_row);
			y += h_row;
			vCenter.SetDouble(ax.in_center(1));
			vRange.SetDouble(ax.in_range(1)*2.0);
		}
		if (nin < 1)
		{
			HIDE(uvDelta);
		}
		else
		{
			MOVE(uvDelta, x1, x3 - SPC, y, h_delta, h_row);
			y += h_row;
			y += 2 * SPC;
		}

		if (!in3d)
		{
			HIDE(phiLabel); HIDE(psiLabel); HIDE(thetaLabel);
			HIDE(phi); HIDE(psi); HIDE(theta);
			HIDE(phiDelta); HIDE(psiDelta); HIDE(thetaDelta);
			HIDE(distLabel); HIDE(dist); HIDE(distDelta);
			HIDE(center); HIDE(top); HIDE(front);
		}
		else
		{
			MOVE(phiLabel, x1, x2 - SPC, y, h_label, h_row);
			MOVE(psiLabel, x2, x3 - SPC, y, h_label, h_row);
			MOVE(thetaLabel, x3, x4 - SPC, y, h_label, h_row);
			y += h_row;
			MOVE(phi, x1, x2 - SPC, y, h_edit, h_row);
			MOVE(psi, x2, x3 - SPC, y, h_edit, h_row);
			MOVE(theta, x3, x4 - SPC, y, h_edit, h_row);
			y += h_row;
			MOVE(phiDelta, x1, x2 - SPC, y, h_delta, h_row);
			MOVE(psiDelta, x2, x3 - SPC, y, h_delta, h_row);
			MOVE(thetaDelta, x3, x4 - SPC, y, h_delta, h_row);
			y += h_row;
			phi.SetDouble(cam.phi());
			psi.SetDouble(cam.psi());
			theta.SetDouble(cam.theta());

			MOVE(distLabel, x1, x2 - SPC, y, h_label, h_row);
			MOVE(dist, x2, x3 - SPC, y, h_edit, h_row);
			MOVE(distDelta, x3, x4 - SPC, y, h_delta, h_row);
			y += h_row + 2 * SPC;
			dist.SetDouble(1.0 / cam.zoom());

			d = (W - 2 * (2 * SPC) - 2 * SPC) / 3;
			const int t0 = 2 * SPC, t1 = t0 + d + SPC, t2 = t1 + d + SPC, t3 = W - 2 * SPC;

			MOVE(center, t0, t1 - SPC, y, h_button, h_row);
			MOVE(top, t1, t2 - SPC, y, h_button, h_row);
			MOVE(front, t2, t3 - SPC, y, h_button, h_row);
			y += h_row;
		}

		y += SPC;
	}
	//----------------------------------------------------------------------------------

	EnableScrollBarCtrl(SB_HORZ, FALSE);
	SetScrollSizes(MM_TEXT, CSize(W, y - y0), CSize(W, bounds.Height()), CSize(h_row, h_row));
}

void SideView::UpdateAxis()
{
	CRect bounds; GetClientRect(bounds);
	if (!doc || bounds.Width() < 2) return;
	if (!axis.GetCheck()) return;

	const Plot   &plot = doc->plot;
	const Graph  *g = plot.current_graph();
	const Axis   &ax = plot.axis;
	const Camera &cam = plot.camera;

	const bool in3d = (plot.axis_type() != Axis::Rect);

	xCenter.SetDouble(ax.center(0));
	xRange.SetDouble(ax.range(0)*2.0);

	yCenter.SetDouble(ax.center(1));
	yRange.SetDouble(ax.range(1)*2.0);

	if (in3d)
	{
		zCenter.SetDouble(ax.center(2));
		zRange.SetDouble(ax.range(2)*2.0);
		phi.SetDouble(cam.phi());
		psi.SetDouble(cam.psi());
		theta.SetDouble(cam.theta());
		dist.SetDouble(1.0 / cam.zoom());
	}

	const int nin = g ? g->inRangeDimension() : -1;

	if (nin >= 1)
	{
		uCenter.SetDouble(ax.in_center(0));
		uRange.SetDouble(ax.in_range(0)*2.0);
	}
	if (nin >= 2)
	{
		vCenter.SetDouble(ax.in_center(1));
		vRange.SetDouble(ax.in_range(1)*2.0);
	}
}

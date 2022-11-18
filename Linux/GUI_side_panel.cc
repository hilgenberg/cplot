#include "GUI.h"
#include "imgui/imgui.h"
#include "PlotWindow.h"

static constexpr int slider_flags = ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat|ImGuiSliderFlags_NoInput;

#define CHKBOOL(g, title, value, action) do{\
	bool on_ = (g), orig = on_ && value, tmp = orig;\
	enable(on_);\
	ImGui::Checkbox(title, &tmp);\
	if (enable && tmp != orig) w.action(); }while(0)

#define SLIDER_WITH_ID(g, id, title, value, min, max, action) do{\
	bool on_ = (g); double v0 = min, v1 = max;\
	auto orig = on_ ? value : min, tmp = orig;\
	enable(on_);\
	ImGui::SliderScalar("##" id, ImGuiDataType_Double, &tmp, &v0, &v1, title, slider_flags);\
	if (enable && tmp != orig) w.action(tmp); }while(0)

#define SLIDER(g, title, value, min, max, action) SLIDER_WITH_ID(g, title, title, value, min, max, action)

#define POPUP(g, title, T, value, action, ...) do{\
	static const char *items[] = {__VA_ARGS__};\
	bool on_ = (g); enable(on_);\
	int orig = on_ ? value : 0, tmp = orig;\
	ImGui::Combo(title, &tmp, items, IM_ARRAYSIZE(items));\
	if (enable && tmp != orig) w.action((T)tmp); }while(0)

#define COLOR(g, title, value, action) do{\
	bool on_ = (g);\
	GL_Color orig = on_ ? value : GL_Color(0.4), tmp = orig;\
	enable(on_);\
	ImGui::ColorEdit4(title, tmp.v, colorEditFlags);\
	if (enable && tmp != orig) w.action(tmp); }while(0)

void GUI::side_panel()
{
	Plot &plot = w.plot;
	if (plot.axis_type() == Axis::Invalid) return;

	ImGuiViewport &screen = *ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(screen.WorkPos.x, screen.WorkPos.y+top_panel_height));
	ImGui::SetNextWindowSize(ImVec2(0.0f, screen.WorkSize.y-top_panel_height-w.status_height()));
	ImGui::SetNextWindowBgAlpha(0.7f);
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoSavedSettings;

	const Graph *g    = plot.current_graph();
	const bool sel    = g != NULL;
	const bool vf     = g && g->isVectorField();
	const bool color  = g && g->isColor();
	const bool line   = g && g->isLine();
	const bool histo  = g && g->isHistogram();
	const bool twoD   = plot.axis_type() == Axis::Rect;
	const bool points = g && g->options.shading_mode == Shading_Points;
	
	EnableGuard enable;

	ImGui::Begin("Side Panel", &show_side_panel, window_flags);

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	if (!vf && (!color || g->mode() == GM_RiemannColor))
		SLIDER(sel, "Quality", g->options.quality, 0.0, 0.6, setQuality);
	if (!color && !line)
		SLIDER(sel && (!histo || !points), "Grid Density", g->options.grid_density, 0.0, 100.0, setGridDensity);
	if (!vf && !color && !histo && !points)
		CHKBOOL(sel, "Detect Discontinuities", g->options.disco, toggleDisco);

	CHKBOOL(1, "Show Axis", !plot.axis.options.hidden, toggleAxis);
	if (twoD)
		POPUP(1, "##Axis Grid", AxisOptions::AxisGridMode, plot.axis.options.axis_grid, setAxisGrid, "Axis Grid Off", "Axis Grid On", "Polar Axis Grid");

	if (vf)
		POPUP(1, "##Vector Field Mode", VectorfieldMode, g->options.vf_mode, setVFMode, "Unscaled Vectors", "Normalized Vectors", "Direction Vectors", "Connected Vector Field");
	else if (!line && !color)
		POPUP(sel, "##Display Mode", ShadingMode, g->options.shading_mode, setDisplayMode, "Points", "Wireframe", "Hiddenline", "Flatshaded", "Smoothshaded");
	if (histo)
	{
		POPUP(1, "##Histogram Mode", HistogramMode, g->options.hist_mode, setHistoMode, "Riemann Histogram", "Disc Histogram", "Normal Histogram");
		if (g->options.hist_mode != HM_Riemann)
		{
			static constexpr double HISTO_MAX  = 1.0e4;
			double v0 = 0.0, v1 = 1.0;
			auto orig = g ? log(g->options.hist_scale*(HISTO_MAX - 2.0) + 1.0)*0.5/log(HISTO_MAX - 1.0) : 0.0, tmp = orig;
			enable(true);
			ImGui::SliderScalar("##Histogram Scale", ImGuiDataType_Double, &tmp, &v0, &v1, "Histogram Scale", slider_flags);
			if (enable && tmp != orig) w.setHistoScale((exp(2.0*tmp*log(HISTO_MAX - 1.0)) - 1.0) / (HISTO_MAX - 2.0));
		}
	}

	if (!color && !line)
	{
		if (!vf && !points)
		{
			POPUP(sel, "##Grid", GridStyle, g->options.grid_style, setGrid, "Hide Grid", "Draw Grid", "Full Grid");
			bool on = g && !histo && g->hasFill();
			static const char *items[] = {
				"Mesh Off", "Chess", "HLines", "VLines", "Circles", "Squares",
				"Triangles", "Rounded Rect", "Rings", "Fan", "Static", "Hexagon",
				"Custom"
			};
			static const MaskStyle order[] = {
				Mask_Off, Mask_Chessboard, Mask_HLines, Mask_VLines, Mask_Circles, Mask_Squares,
				Mask_Triangles, Mask_Rounded_Rect, Mask_Rings, Mask_Fan, Mask_Static, Mask_Hexagon,
				Mask_Custom
			};
			static const int index[] = {0, 4, 5, 6, 7, 1, 2, 3, 8, 10, 9, 11, 12};
			int N = IM_ARRAYSIZE(items);
			assert(IM_ARRAYSIZE(items) == IM_ARRAYSIZE(order));
			assert(IM_ARRAYSIZE(index) == IM_ARRAYSIZE(order));
			#ifndef NDEBUG
			for (int i = 0; i < N-1; ++i) 
			{
				assert(index[order[i]] == i);
				assert(order[index[i]] == i);
			}
			#endif
			bool custom = g->options.mask.style() == Mask_Custom;
			if (!custom) --N; // hide last entry
			int orig = on ? (custom ? 12 : index[g->options.mask.style()]) : 0, tmp = orig;
			enable(on);
			ImGui::Combo("##Mesh", &tmp, items, N);
			if (enable && tmp != orig) w.setMask(order[tmp]);
		}
		if (g && g->hasFill() && g->options.mask.style() != Mask_Off)
			SLIDER(1, "Alpha Mask Cutoff", g->options.mask.density(), 0.0, 1.0, setMaskParam);
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	bool on = (g && !histo && plot.axis_type() == Axis::Box && g->type() != R3_R);
	CHKBOOL(on, "Clip to Axis", g->clipping(), toggleClip);
	
	if (!twoD)
	{
		CHKBOOL(1, "Custom Clipping Plane", plot.options.clip.on(), toggleClipCustom);
		if (plot.options.clip.on())
		{
			CHKBOOL(1, "Lock", plot.options.clip.locked(), toggleClipLock);
			ImGui::SameLine(0.0f, 10.0f);
			enable(plot.options.clip.locked());
			if (ImGui::Button("Reset")) w.resetClipLock();

			enable(true);
			float v0 = -1.0, v1 = 1.0;
			float orig = plot.options.clip.distance(), tmp = orig;
			ImGui::SliderFloat("##Clip Distance", &tmp, v0, v1, "Distance", slider_flags);
			if (tmp != orig) w.setClipDistance(tmp);
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	//---------------------------------------------------------------------------

	//ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7);
	static constexpr int colorEditFlags = 
		ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs |
		ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaBar |
		ImGuiColorEditFlags_PickerHueWheel;
	COLOR(1, "Background", plot.axis.options.background_color, setBgColor);
	COLOR(1, "Axis", plot.axis.options.axis_color, setAxisColor);
	COLOR(g && g->hasFill(), "Fill", g->options.fill_color, setFillColor);
	COLOR(g && !color, g && g->usesLineColor() ? "Line" :"Grid", 
		(g->usesLineColor() ? g->options.line_color : g->options.grid_color), setGridColor);
	//ImGui::PopItemWidth();

	if (!vf && !color && !line && !twoD)
		SLIDER(g && g->hasNormals(), "Shinyness", g->options.shinyness, 0.0, 1.0, setShinyness);

	if (!vf && !line && (!sel || color || g->hasFill()))
	{
		const bool on = sel && (color || g->hasFill());
		static const char *items[] = {
			"Blending Off", "Alpha Blending", "Additive", "Subtractive",
			"Multiply", "Glass", "Custom Blending" };
		auto &tm = DefaultBlendModes(); int n = (int)tm.size();
		assert(IM_ARRAYSIZE(items) == n+1);
		enable(on);
		int orig = -1;
		if (g) for (int i = 0; i < n; ++i)
		{
			if (g->options.transparency == tm[i].mode)
			{
				orig = i;
				break;
			}
		}
		if (g && orig < 0) { orig = n; }
		int tmp = orig;
		ImGui::Combo("##BlendingMode", &tmp, items, std::max(n, orig+1));
		if (enable && tmp != orig) w.setTransparencyMode(tm[tmp].mode);
	}

	if (w.accum_size() > 0)
	{
		POPUP(1, "##Antialiasing", AntialiasMode, plot.options.aa_mode, setAAMode, 
		"Antialiasing Off", "Antialias Lines", "4x FSAA", "8x FSAA", "4x FSAA Acc", "8x FSAA Acc");
	}
	else
	{
		POPUP(1, "##Antialiasing", AntialiasMode, plot.options.aa_mode, setAAMode, 
		"Antialiasing Off", "Antialias Lines", "4x FSAA", "8x FSAA");
	}

	if (!vf && !color)
	{
		const bool on = g && (line || g->options.shading_mode == Shading_Wireframe ||
			points || g->options.grid_style != Grid_Off);
		SLIDER_WITH_ID(on, "Point Size Line Width",
			(g && g->isArea() && g->usesShading() && points) ? "Point Size" :"Line Width",
			g->usesLineColor() ? g->options.line_width : g->options.gridline_width, 0.0, 5.0, setLineWidth);
	}
	
	if (!twoD)
		SLIDER(1, "Fog", plot.options.fog, 0.0, 1.0, setFog);

	if (!vf && !line && !points)
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		const bool hasTex = g && (color || g->hasFill());
		const bool hasRef = g && !color && !twoD && g->hasNormals();

		SLIDER(hasTex, "Texture", g->options.texture_opacity, 0.0, 1.0, setTextureStrength);
		static const char *tex_items[] = { "Custom", "Color", "XOR", "Grey XOR", "Checker", "Plasma", "Color2", "Phase", "Units" };
		{
			int orig = g ? g->options.texture.is_pattern() : -10;
			int d = (orig==0 ? 0 : 1); orig -= d;
			int tmp = orig;
			enable(hasTex);
			ImGui::Combo("##TextureCombo", &tmp, tex_items+d, IM_ARRAYSIZE(tex_items)-d);
			if (enable && tmp != orig) w.setTexture((GL_ImagePattern)(tmp+d));
		}
		if (color) POPUP(1, "##TextureMode", TextureProjection, g->options.texture_projection, setTextureMode, 
			"Tiled Texture", "Centered  Texture", "Riemann Texture", "Spherical Texture");

		ImGui::Spacing();
		ImGui::Spacing();

		SLIDER(hasRef, "Reflection", g->options.reflection_opacity, 0.0, 1.0, setReflectionStrength);
		{
			int orig = g ? g->options.reflection_texture.is_pattern() : -10;
			int d = (orig==0 ? 0 : 1); orig -= d;
			int tmp = orig;
			enable(hasRef);
			ImGui::Combo("##ReflectionTextureCombo", &tmp, tex_items+d, IM_ARRAYSIZE(tex_items)-d);
			if (enable && tmp != orig) w.setReflectionTexture((GL_ImagePattern)(tmp+d));
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	//---------------------------------------------------------------------------
	Axis   &ax = plot.axis;
	Camera &cam = plot.camera;
	const bool in3d = (plot.axis_type() != Axis::Rect);
	static constexpr int flags = ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat;
	const char *fmt = "%.3g";

	enable(true);
	static const char *labels[] = {"x", "y", "z"};
	for (int i = 0; i < 2+in3d; ++i)
	{
		double c0 = ax.center(i), c = c0;
		double r0 = ax.range(i), r = r0;
		ImGui::PushID(labels[i]);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
		ImGui::DragScalar("##center", ImGuiDataType_Double, &c, (float)std::max(1e-6, 0.01*r0), NULL, NULL, fmt, flags);
		if (c != c0 && defined(c))
		{
			w.undoForAxis();
			plot.axis.center(i, c);
			w.recalc(plot);
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##range", ImGuiDataType_Double, &r, std::max(1e-6, 0.01*r0), NULL, NULL, fmt, flags);
		if (r != r0 && defined(r))
		{
			w.undoForAxis();
			if (i == 1 && !in3d)
				plot.axis.range(0, plot.axis.range(0)*r/r0);
			else
				plot.axis.range(i, r);
			w.recalc(plot);
		}
		ImGui::PopID();
	}
	if (in3d)
	{
		double x = 0.0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##range", ImGuiDataType_Double, &x, 0.01f, NULL, NULL, "axis range", flags|ImGuiSliderFlags_NoInput);
		if (x != 0.0 && defined(x))
		{
			w.undoForAxis();
			x = exp(x);
			for (int i = 0; i < 2+in3d; ++i)
			{
				ax.range(i, ax.range(i)*x);
			}
			w.recalc(plot);
		}
	}

	const int nin = g ? g->inRangeDimension() : -1;
	if (nin > 0)
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	}
	static const char *in_labels[] = {"u", "v"};
	for (int i = 0; i < nin; ++i)
	{
		double c0 = ax.in_center(i), c = c0;
		double r0 = ax.in_range(i), r = r0;
		ImGui::PushID(in_labels[i]);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
		ImGui::DragScalar("##center", ImGuiDataType_Double, &c, (float)std::max(1e-6, 0.01*r0), NULL, NULL, fmt, flags);
		if (c != c0 && defined(c))
		{
			w.undoForInRange();
			plot.axis.in_center(i, c);
			w.recalc(plot);
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##range", ImGuiDataType_Double, &r, std::max(1e-6, 0.01*r0), NULL, NULL, fmt, flags);
		if (r != r0 && defined(r))
		{
			w.undoForInRange();
			plot.axis.in_range(i, r);
			w.recalc(plot);
		}
		ImGui::PopID();
	}
	if (nin > 1)
	{
		double x = 0.0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##in_range", ImGuiDataType_Double, &x, 0.01f, NULL, NULL, "input range", flags|ImGuiSliderFlags_NoInput);
		if (x != 0.0 && defined(x))
		{
			w.undoForInRange();
			x = exp(x);
			for (int i = 0; i < nin; ++i)
			{
				ax.in_range(i, ax.in_range(i)*x);
			}
			w.recalc(plot);
		}
	}

	if (in3d)
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		double v0 = cam.phi(), v = v0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.33);
		ImGui::DragScalar("##phi", ImGuiDataType_Double, &v, 1.0f, NULL, NULL, "%.2f", flags);
		if (v != v0 && defined(v))
		{
			w.undoForCam();
			cam.set_phi(v);
			w.redraw();
		}
		ImGui::SameLine();
		v0 = cam.psi(); v = v0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5);
		ImGui::DragScalar("##psi", ImGuiDataType_Double, &v, 1.0f, NULL, NULL, "%.2f", flags);
		if (v != v0 && defined(v))
		{
			w.undoForCam();
			cam.set_psi(v);
			w.redraw();
		}
		ImGui::SameLine();
		v0 = cam.theta(); v = v0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##theta", ImGuiDataType_Double, &v, 1.0f, NULL, NULL, "%.2f", flags);
		if (v != v0 && defined(v))
		{
			w.undoForCam();
			cam.set_theta(v);
			w.redraw();
		}

		v0 = 1.0 / cam.zoom(); v = v0;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragScalar("##zoom", ImGuiDataType_Double, &v, (float)(0.01*v0), NULL, NULL, "zoom", flags|ImGuiSliderFlags_NoInput);
		if (v != v0 && defined(v) && v > 0)
		{
			w.undoForCam();
			cam.set_zoom(1.0 / v);
			w.redraw();
		}
	}

	enable(true);
	ImGui::PopItemWidth();
	ImGui::End();
}

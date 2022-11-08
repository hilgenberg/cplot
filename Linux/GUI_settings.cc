#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

static constexpr int slider_flags = ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat|ImGuiSliderFlags_NoInput;

#define CHKBOOL(g, title, value, action) do{\
	bool on_ = (g), orig = on_ && value, tmp = orig;\
	enable(on_);\
	ImGui::Checkbox(title, &tmp);\
	if (enabled && tmp != orig) w.action(); }while(0)

#define SLIDER_WITH_ID(g, id, title, value, min, max, action) do{\
	bool on_ = (g); double v0 = min, v1 = max;\
	auto orig = on_ ? value : min, tmp = orig;\
	enable(on_);\
	ImGui::SliderScalar("##" id, ImGuiDataType_Double, &tmp, &v0, &v1, title, slider_flags);\
	if (enabled && tmp != orig) w.action(tmp); }while(0)

#define SLIDER(g, title, value, min, max, action) SLIDER_WITH_ID(g, title, title, value, min, max, action)

#define POPUP(g, title, T, value, action, ...) do{\
	static const char *items[] = {__VA_ARGS__};\
	bool on_ = (g); enable(on_);\
	int orig = on_ ? value : 0, tmp = orig;\
	ImGui::Combo(title, &tmp, items, IM_ARRAYSIZE(items));\
	if (enabled && tmp != orig) w.action((T)tmp); }while(0)

#define COLOR(g, title, value, action) do{\
	bool on_ = (g);\
	GL_Color orig = on_ ? value : GL_Color(0.4), tmp = orig;\
	enable(on_);\
	ImGui::ColorEdit4(title, tmp.v, colorEditFlags);\
	if (enabled && tmp != orig) w.action(tmp); }while(0)

void GUI::settings_panel()
{
	const Plot &plot = w.plot;
	if (plot.axis_type() == Axis::Invalid) return;

	ImGuiViewport &screen = *ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(screen.WorkPos.x, screen.WorkPos.y+main_panel_height));
	ImGui::SetNextWindowSize(ImVec2(0.0f, screen.WorkSize.y-main_panel_height));
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

	ImGui::Begin("Settings", &show_settings_panel, window_flags);
	assert(enabled);

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
			if (enabled && tmp != orig) w.setHistoScale((exp(2.0*tmp*log(HISTO_MAX - 1.0)) - 1.0) / (HISTO_MAX - 2.0));
		}
	}

	if (!color && !line)
	{
		if (!vf && !points)
		{
			POPUP(sel, "##Grid", GridStyle, g->options.grid_style, setGrid, "Hide Grid", "Draw Grid", "Full Grid");
			bool on = g && !histo && g->hasFill();
			if (g && g->options.mask.style() == Mask_Custom)
			{
				ImGui::Text("TODO: Custom Mesh UI");
			}
			else
			{
				static const char *items[] = {
					"Mesh Off", "Chess", "HLines", "VLines", "Circles", "Squares",
					"Triangles", "Rounded Rect", "Rings", "Fan", "Static", "Hexagon"
				};
				static const MaskStyle order[] = {
					Mask_Off, Mask_Chessboard, Mask_HLines, Mask_VLines, Mask_Circles, Mask_Squares,
					Mask_Triangles, Mask_Rounded_Rect, Mask_Rings, Mask_Fan, Mask_Static, Mask_Hexagon
				};
				static const int index[] = {0, 4, 5, 6, 7, 1, 2, 3, 8, 10, 9, 11};

				assert(IM_ARRAYSIZE(items) == IM_ARRAYSIZE(order));
				assert(IM_ARRAYSIZE(index) == IM_ARRAYSIZE(order));
				#ifndef NDEBUG
				for (int i = 0; i < IM_ARRAYSIZE(index); ++i) 
				{
					assert(index[order[i]] == i);
					assert(order[index[i]] == i);
				}
				#endif

				int orig = on ? index[g->options.mask.style()] : 0, tmp = orig;
				enable(on);
				ImGui::Combo("##Mesh", &tmp, items, IM_ARRAYSIZE(items));
				if (enabled && tmp != orig) w.setMeshMode(order[tmp]);
			}
		}
		if (g && g->hasFill() && g->options.mask.style() != Mask_Off)
			SLIDER(1, "Mesh density", g->options.mask.density(), 0.0, 1.0, setMeshDensity);
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
		if (enabled && tmp != orig) w.setTransparencyMode(tm[tmp].mode);
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
			if (enabled && tmp != orig) w.setTexture((GL_ImagePattern)(tmp+d));
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
			if (enabled && tmp != orig) w.setReflectionTexture((GL_ImagePattern)(tmp+d));
		}
	}

	enable();
	ImGui::PopItemWidth();
	ImGui::End();
}

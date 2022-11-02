#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

static bool show_demo_window = false;
static bool show_another_window = false;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

GUI::GUI(SDL_Window* window, SDL_GLContext context, PlotWindow &w)
: w(w)
, visible(false)
, need_redraw(1)
, enabled(true)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.MouseDrawCursor = true;
	io.ConfigInputTextCursorBlink = false;
	io.ConfigInputTrickleEventQueue = false;
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL2_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

}
GUI::~GUI()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

bool GUI::handle_event(const SDL_Event &event)
{
	if (!visible) return false;
	if (ImGui_ImplSDL2_ProcessEvent(&event)) redraw();
	ImGuiIO &io = ImGui::GetIO();

	switch (event.type)
	{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (io.WantCaptureKeyboard) return true;
			break;

		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			if (io.WantCaptureMouse) return true;
			break;
	}
	return false;
}

void GUI::update()
{
	if (!visible) return;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	settings_window();

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello World!");
		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}

void GUI::draw()
{
	if (!visible) return;
	ImGui::Render();
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	if (need_redraw > 0) --need_redraw;
}

void GUI::begin_section()
{
	enabled = true;
}
void GUI::end_section()
{
	if (!enabled) ImGui::EndDisabled();
	enabled = true;
}
bool GUI::enable(bool e)
{
	if (e) {
		if (!enabled) ImGui::EndDisabled();
	} else {
		if (enabled) ImGui::BeginDisabled();
	}
	enabled = e;
	return e;
}

#define CHKBOOL(g, title, value, action) do{\
	bool on = (g), orig = on && value, tmp = orig;\
	enable(on);\
	ImGui::Checkbox(title, &tmp);\
	if (enabled && tmp != orig) w.action(); }while(0)

#define SLIDER(g, title, value, min, max, action) do{\
	bool on = (g); double v0 = min, v1 = max;\
	auto orig = on ? value : min, tmp = orig;\
	enable(on);\
	ImGui::SliderScalar("##" title, ImGuiDataType_Double, &tmp, &v0, &v1, title);\
	if (enabled && tmp != orig) w.action(tmp); }while(0)

#define POPUP(g, title, T, value, action, ...) do{\
	static const char *items[] = {__VA_ARGS__};\
	bool on = (g); enable(on);\
	int orig = on ? value : 0, tmp = orig;\
	ImGui::Combo(title, &tmp, items, IM_ARRAYSIZE(items));\
	if (enabled && tmp != orig) w.action((T)tmp); }while(0)

void GUI::settings_window()
{
	begin_section();

	Plot &plot = w.plot;
	Graph *g = plot.current_graph();

	ImGui::Begin("Settings");

	std::string name("Undo");
	enable(w.ut.can_undo(name));
	if (ImGui::Button(name.c_str())) w.ut.undo();
	ImGui::SameLine();
	name = "Redo";
	enable(w.ut.can_redo(name));
	if (ImGui::Button(name.c_str())) w.ut.redo();

	bool invalid = (plot.axis_type() == Axis::Invalid);

	CHKBOOL(!invalid, "Show Axis", !plot.axis.options.hidden, toggleAxis);
	CHKBOOL(g, "Detect Discontinuities", g->options.disco, toggleDisco);
	CHKBOOL(g, "Clip to Axis", g->clipping(), toggleClip);
	CHKBOOL(1, "Custom Clipping Plane", plot.options.clip.on(), toggleClipCustom);
	CHKBOOL(1, "Lock Custom Clipping Plane", plot.options.clip.locked(), toggleClipLock);
	if (ImGui::Button("Reset Clip")) w.resetClipLock();

	POPUP(g, "##Grid", GridStyle, g->options.grid_style, setGrid, "Grid Off", "Grid On", "Grid Full");
	POPUP(1, "##Axis Grid", AxisOptions::AxisGridMode, plot.axis.options.axis_grid, setAxisGrid, "Axis Grid Off", "Axis Grid On", "Polar Axis Grid");
	POPUP(g, "##Display Mode", ShadingMode, g->options.shading_mode, setDisplayMode, "Points", "Wireframe", "Hiddenline", "Flatshaded", "Smoothshaded");
	POPUP(g, "##Vector Field Mode", VectorfieldMode, g->options.vf_mode, setVFMode, "Unscaled Vectors", "Normalized Vectors", "Direction Vectors", "Connected Vector Field");
	
	SLIDER(g, "Quality", g->options.quality, 0.0, 0.1, setQuality);
	SLIDER(g, "Grid Density", g->options.grid_density, 0.0, 100.0, setGridDensity);

	POPUP(g, "##Histogram Mode", HistogramMode, g->options.hist_mode, setHistoMode, "Riemann Histogram", "Disc Histogram", "Normal Histogram");

	{
	static constexpr double HISTO_MAX  = 1.0e4;
	double v0 = 0.0, v1 = 1.0;
	auto orig = g ? log(g->options.hist_scale*(HISTO_MAX - 2.0) + 1.0)*0.5/log(HISTO_MAX - 1.0) : 0.0, tmp = orig;
	enable(g);
	ImGui::SliderScalar("##Histogram Scale", ImGuiDataType_Double, &tmp, &v0, &v1, "Histogram Scale");
	if (enabled && tmp != orig) w.setHistoScale((exp(2.0*tmp*log(HISTO_MAX - 1.0)) - 1.0) / (HISTO_MAX - 2.0));
	}

	{
	float v0 = -1.0, v1 = 1.0;
	float orig = plot.options.clip.distance(), tmp = orig;
	enable(true);
	ImGui::SliderFloat("##Clip Distance", &tmp, v0, v1, "Clip Distance");
	if (enabled && tmp != orig) w.setClipDistance(tmp);
	}

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

		int orig = g ? index[g->options.mask.style()] : 0, tmp = orig;
		enable(g);
		ImGui::Combo("##Mesh", &tmp, items, IM_ARRAYSIZE(items));
		if (enabled && tmp != orig) w.setMeshMode(order[tmp]);
	}

	SLIDER(g, "Mesh density", g->options.mask.density(), 0.0, 1.0, setMeshDensity);


	//ImGui::SameLine();
	end_section();
	ImGui::End();
}



















#if 0


int SideSectionSettings::OnCreate(LPCREATESTRUCT cs)
{

	BUTTON(clipReset, "Reset");

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

#endif

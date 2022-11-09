#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"
#include "../Utility/Preferences.h"

#define CHKBOOL(g, title, value, action) do{\
	bool on_ = (g), orig = on_ && value, tmp = orig;\
	enable(on_);\
	ImGui::Checkbox(title, &tmp);\
	if (enabled && tmp != orig) w.action(); }while(0)

static constexpr int slider_flags = ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat|ImGuiSliderFlags_NoInput;
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

void GUI::prefs_panel()
{
	if (!show_prefs_panel) return;

	ImGui::Begin("Preferences", &show_prefs_panel);
	assert(enabled);

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

	bool b0 = Preferences::drawNormals(), b = b0;
	ImGui::Checkbox("Draw Normals", &b);
	if (b != b0)
	{
		Preferences::drawNormals(b);
		redraw();
	}
	
	b0 = Preferences::dynamic(); b = b0;
	ImGui::Checkbox("Reduce Quality During Animation", &b);
	if (b != b0)
	{
		Preferences::dynamic(b);
		redraw();
	}

	enable();
	ImGui::PopItemWidth();
	if (ImGui::Button("Done")) show_prefs_panel = false;

	ImGui::End();
}

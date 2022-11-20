#include "GUI.h"
#include "imgui/imgui.h"
#include "../Utility/Preferences.h"

void GUI::prefs_panel()
{
	if (!show_prefs_panel) return;

	ImGui::Begin("Preferences", &show_prefs_panel);
	bool b0, b; int i0, i;

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	
	b0 = Preferences::drawNormals(); b = b0;
	ImGui::Checkbox("Draw Normals", &b);
	if (b != b0) { Preferences::drawNormals(b); redraw(); }
	
	b0 = Preferences::showFPS(); b = b0;
	ImGui::Checkbox("Show FPS", &b);
	if (b != b0) { Preferences::showFPS(b); redraw(); }
	
	ImGui::Text("Animation FPS limit (-1 for none)");
	i0 = Preferences::fps(); i = i0;
	ImGui::InputInt("##fps", &i, 1, 0);
	if (i != i0) Preferences::fps(i);

	b0 = Preferences::vsync(); b = b0;
	ImGui::Checkbox("VSync", &b);
	if (b != b0) {
		SDL_GL_SetSwapInterval(b);
		Preferences::vsync(b); redraw();
	}
	
	b0 = Preferences::dynamic(); b = b0;
	ImGui::Checkbox("Reduce Quality During Animation", &b);
	if (b != b0) { Preferences::dynamic(b); redraw(); }

	b0 = Preferences::depthSort(); b = b0;
	ImGui::Checkbox("Depth Sorting", &b);
	if (b != b0) { Preferences::depthSort(b); redraw(); }

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Text("Number of Threads (-1 for default = %d)", n_cores);
	i0 = Preferences::threads(false); i = i0;
	ImGui::InputInt("##threads", &i, 1, 0);
	if (i != i0) Preferences::threads(i);

	ImGui::PopItemWidth();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	if (ImGui::Button("Done")) show_prefs_panel = false;

	ImGui::End();
}

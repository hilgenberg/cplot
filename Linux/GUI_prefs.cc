#include "GUI.h"
#include "imgui/imgui.h"
#include "../Utility/Preferences.h"
#include "../Utility/System.h"

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

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Text("Number of Threads (-1 for default = %d)", n_cores);
	int i0 = Preferences::threads(false), i = i0;
	ImGui::InputInt("##threads", &i, 1, 0);
	if (i != i0) Preferences::threads(i);

	ImGui::PopItemWidth();
	enable();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	if (ImGui::Button("Done")) show_prefs_panel = false;

	ImGui::End();
}

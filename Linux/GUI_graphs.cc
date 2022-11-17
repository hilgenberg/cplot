#include "GUI.h"
#include "imgui/imgui.h"
#include "PlotWindow.h"

struct GUI_GraphsMenu : public GUI_Menu
{
	GUI_GraphsMenu(GUI &gui) : GUI_Menu(gui) {}

	void operator()()
	{
		if (!ImGui::BeginMenu("Graphs")) return;
		const Plot &plot = gui.w.plot;
		const Graph *g0 = plot.current_graph();
		const int n = plot.number_of_graphs();
		for (int i = 0; i < n; ++i)
		{
			Graph *g = plot.graph(i);
			ImGui::PushID(g);
			bool sel = (g == g0);
			std::string desc = g->description_line();
			if (g->options.hidden) desc += " (hidden)";
			if (ImGui::MenuItem(desc.c_str(), NULL, sel)) gui.w.selectGraph(i);
			ImGui::PopID();
		}
		if (n > 0) ImGui::Separator();

		if (ImGui::MenuItem("Add Graph")) gui.w.addGraph();
		if (ImGui::MenuItem("Delete Selected Graph", NULL, false, g0 != NULL)) gui.w.deleteGraph(g0->oid());
		if (ImGui::MenuItem((g0 && g0->options.hidden) ? "Show Selected Graph" : "Hide Selected Graph",
			NULL, false, g0 != NULL)) gui.w.toggleGraphVisibility();
		ImGui::EndMenu();
	}
};
GUI_Menu *new_graphs_menu(GUI &gui) { return new GUI_GraphsMenu(gui); }

#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

void GUI::graphs_menu()
{
	if (!ImGui::BeginMenu("Graphs")) return;
	const Plot &plot = w.plot;
	const Graph *g0 = plot.current_graph();
	const int n = plot.number_of_graphs();
	for (int i = 0; i < n; ++i)
	{
		Graph *g = plot.graph(i);
		ImGui::PushID(g);
		bool sel = (g == g0);
		std::string desc = g->description_line();
		if (g->options.hidden) desc += " (hidden)";
		if (ImGui::MenuItem(desc.c_str(), NULL, sel)) w.selectGraph(i);
		ImGui::PopID();
	}
	if (n > 0) ImGui::Separator();

	if (ImGui::MenuItem("Add Graph")) w.addGraph();
	if (ImGui::MenuItem("Delete Selected Graph", NULL, false, g0 != NULL)) w.deleteGraph(g0->oid());
	if (ImGui::MenuItem((g0 && g0->options.hidden) ? "Show Selected Graph" : "Hide Selected Graph",
		NULL, false, g0 != NULL)) w.toggleGraphVisibility();
	ImGui::EndMenu();
}

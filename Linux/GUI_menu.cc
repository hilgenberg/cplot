#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

void GUI::main_menu()
{
	if (!ImGui::BeginMainMenuBar()) return;

	file_menu();

	if (ImGui::BeginMenu("Edit")) {
		std::string name("Undo"); bool on = w.ut.can_undo(name);
		name += "###UndoItem";
		if (ImGui::MenuItem(name.c_str(), "CTRL+Z", false, on)) w.ut.undo();

		name = "Redo"; on = w.ut.can_redo(name);
		name += "###RedoItem";
		if (ImGui::MenuItem(name.c_str(), "CTRL+Y", false, on)) w.ut.redo();

		ImGui::Separator();
		if (ImGui::MenuItem("Cut", "CTRL+X")) {}
		if (ImGui::MenuItem("Copy", "CTRL+C")) {}
		if (ImGui::MenuItem("Paste", "CTRL+V")) {}
		ImGui::EndMenu();
	}

	graphs_menu();
	params_menu();
	defs_menu();
	axis_menu();

	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem("Show Main Panel", NULL, &show_main_panel);
		ImGui::MenuItem("Show Settings Panel", NULL, &show_settings_panel);

		#ifdef DEBUG
		ImGui::Separator();
		ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
		#endif

		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

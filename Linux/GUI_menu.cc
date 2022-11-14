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

	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem("Show Main Panel", "CTRL-F1", &show_main_panel);
		ImGui::MenuItem("Show Settings Panel", "CTRL-F2", &show_settings_panel);

		#ifdef DEBUG
		ImGui::Separator();
		ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
		#endif

		ImGui::Separator();
		if (ImGui::MenuItem("Center Axis"))
		{
			w.undoForAxis();
			w.plot.axis.reset_center();
			w.recalc(w.plot);
		}
		if (ImGui::MenuItem("Equal Axis Ranges"))
		{
			w.undoForAxis();
			w.plot.axis.equal_ranges();
			w.recalc(w.plot);
		}
		auto ChangeView = [this](double a, double b, double c)
		{
			w.undoForCam();
			w.plot.camera.set_angles(a, b, c);
			w.redraw();
		};
		if (ImGui::MenuItem("View Top"))    { ChangeView(  0.0,  90.0, 0.0); }
		if (ImGui::MenuItem("View Bottom")) { ChangeView(  0.0, -90.0, 0.0); }
		if (ImGui::MenuItem("View Front"))  { ChangeView(  0.0,   0.3, 0.0); }
		if (ImGui::MenuItem("View Back"))   { ChangeView(180.0,   0.3, 0.0); }
		if (ImGui::MenuItem("View Left"))   { ChangeView( 90.0,   0.3, 0.0); }
		if (ImGui::MenuItem("View Right"))  { ChangeView(-90.0,   0.3, 0.0); }

		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

bool GUI::handle_event(const SDL_Event &event)
{
	if (visible && ImGui_ImplSDL2_ProcessEvent(&event)) redraw();
	ImGuiIO &io = ImGui::GetIO();

	bool activate = false, handled = false;
	if (event.type == SDL_KEYDOWN)
	{
		auto key = event.key.keysym.sym;
		auto m   = event.key.keysym.mod;
		constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
		const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
		const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
		const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
		const int  mods  = shift + ctrl + alt;

		switch (key)
		{
			case SDLK_COMMA:
				if (mods == CTRL)
				{ 
					show_prefs_panel = true;
					activate = handled = true;
				}
				break;
			case SDLK_z:
				if (mods == CTRL) { w.ut.undo(); return true; }
				break;
			case SDLK_y:
				if (mods == CTRL) { w.ut.redo(); return true; }
				break;
			case SDLK_l:
				if (mods == CTRL)
				{
					ImGui::SetWindowFocus("Graph Definition");
					// TODO: focus the input field
					activate = handled = true;
				}
				break;
			case SDLK_s:
				if (mods == CTRL+SHIFT)
				{
					save_as();
					activate = handled = true;
				}
				else if (mods == CTRL)
				{
					save();
					handled = true;
				}
				break;
			case SDLK_o:
				if (mods == CTRL)
				{
					open_file();
					activate = handled = true;
				}
				break;
			case SDLK_TAB:
			case SDLK_SPACE:
			case SDLK_RETURN:
				if (mods || visible) break;
				activate = handled = true;
				break;
			case SDLK_F1: case SDLK_F2: case SDLK_F3: case SDLK_F4:
			case SDLK_F5: case SDLK_F6: case SDLK_F7: case SDLK_F8:
			case SDLK_F9: case SDLK_F10: case SDLK_F11: case SDLK_F12:
			{
				int i = key - SDLK_F1; assert(i >= 0 && i < 12);
				if (mods == CTRL)
				{
					if (i > 1) break;
					bool &b = (i == 0 ? show_main_panel : show_settings_panel);
					if (visible) b = !b;
					activate = handled = true;
				}
				else if (mods == 0)
				{
					const Plot &plot = w.plot;
					const Graph *g = plot.current_graph(); if (!g) break;
					auto M = g->valid_modes();
					if (i < M.size()) w.setMode(M[i]);
					handled = true;
				}
				break;
			}

			case SDLK_ESCAPE:
				if (mods) break;
				toggle();
				redraw();
				w.redraw();
				return true;
		}
	}

	if (activate)
	{
		visible = true;
		redraw();
	}

	if (visible) switch (event.type)
	{
		case SDL_KEYDOWN: case SDL_KEYUP:
			if (io.WantCaptureKeyboard) return true;
			break;
		case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL:
			if (io.WantCaptureMouse) return true;
			break;
	}
	return handled;
}

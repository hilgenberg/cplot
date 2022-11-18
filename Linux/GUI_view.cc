#include "GUI.h"
#include "imgui/imgui.h"
#include "PlotWindow.h"

struct GUI_ViewMenu : public GUI_Menu
{
	GUI_ViewMenu(GUI &gui) : GUI_Menu(gui) {}

	void view(double a, double b)
	{
		auto &w = gui.w;
		switch (w.plot.axis_type())
		{
			case Axis::Invalid:
			case Axis::Rect: return;
			default: break;
		}
		w.undoForCam();
		w.plot.camera.set_angles(a, b, 0.0);
		w.redraw();
	};

	void operator()()
	{
		if (!ImGui::BeginMenu("View")) return;
		auto &w = gui.w;
	
		ImGui::MenuItem("Show Top Panel",  "CTRL-F1", &gui.show_top_panel);
		ImGui::MenuItem("Show Side Panel", "CTRL-F2", &gui.show_side_panel);
		ImGui::MenuItem("Show Help",       "CTRL-F3", &gui.show_help_panel);

		#ifdef DEBUG
		ImGui::MenuItem("Show Demo Window", NULL, &gui.show_demo_window);
		#endif

		ImGui::Separator();
		if (ImGui::MenuItem("Center Axis", "Z"))
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

		if (ImGui::MenuItem("View Top",          "T")) { view(  0.0,  90.0); }
		if (ImGui::MenuItem("View Bottom", "Shift+T")) { view(  0.0, -90.0); }
		if (ImGui::MenuItem("View Front",        "F")) { view(  0.0,   0.3); }
		if (ImGui::MenuItem("View Back",   "Shift+F")) { view(180.0,   0.3); }
		if (ImGui::MenuItem("View Left",         "S")) { view( 90.0,   0.3); }
		if (ImGui::MenuItem("View Right",  "Shift+S")) { view(-90.0,   0.3); }

		ImGui::EndMenu();
	}

	bool handle(const SDL_Event &event)
	{
		if (event.type != SDL_KEYDOWN) return false;

		auto &w = gui.w;
		auto key = event.key.keysym.sym;
		auto m   = event.key.keysym.mod;
		constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
		const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
		const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
		const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
		const int  mods  = shift + ctrl + alt;

		switch (key)
		{
			case SDLK_F1: case SDLK_F2: case SDLK_F3:
			{
				if (mods != CTRL) break;
				bool *b = NULL;
				switch (key)
				{
					case SDLK_F1: b = &gui.show_top_panel; break;
					case SDLK_F2: b = &gui.show_side_panel; break;
					case SDLK_F3: b = &gui.show_help_panel; break;
				}
				if (!gui.visible) { *b = true; gui.show(); return true; }
				if (b) *b = !*b;
				gui.redraw();
				return true;
			}

			case SDLK_t: if (ctrl||alt) return false; view(0.0, shift ? -90.0 : 90.0); return true;
			case SDLK_f: if (ctrl||alt) return false; view(shift ? 180.0 : 0.0,  0.3); return true;
			case SDLK_s: if (ctrl||alt) return false; view(shift ? 90.0 : -90.0, 0.3); return true;

			case SDLK_z:
				if (mods) break;
				w.undoForAxis();
				w.plot.axis.reset_center();
				w.recalc(w.plot);
				return true;
		}
		return false;
	}
};
GUI_Menu *new_view_menu(GUI &gui) { return new GUI_ViewMenu(gui); }

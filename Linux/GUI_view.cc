#include "GUI.h"
#include "imgui/imgui.h"
#include "PlotWindow.h"

struct GUI_ViewMenu : public GUI_Menu
{
	GUI_ViewMenu(GUI &gui) : GUI_Menu(gui) {}

	void operator()()
	{
		if (!ImGui::BeginMenu("View")) return;
		auto &w = gui.w;
	
		ImGui::MenuItem("Show Top Panel", "CTRL-F1", &gui.show_top_panel);
		ImGui::MenuItem("Show Side Panel", "CTRL-F2", &gui.show_side_panel);

		#ifdef DEBUG
		ImGui::Separator();
		ImGui::MenuItem("Demo Window", NULL, &gui.show_demo_window);
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

		auto ChangeView = [this](double a, double b)
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
		if (ImGui::MenuItem("View Top",          "T")) { ChangeView(  0.0,  90.0); }
		if (ImGui::MenuItem("View Bottom", "Shift+T")) { ChangeView(  0.0, -90.0); }
		if (ImGui::MenuItem("View Front",        "F")) { ChangeView(  0.0,   0.3); }
		if (ImGui::MenuItem("View Back",   "Shift+F")) { ChangeView(180.0,   0.3); }
		if (ImGui::MenuItem("View Left",         "S")) { ChangeView( 90.0,   0.3); }
		if (ImGui::MenuItem("View Right",  "Shift+S")) { ChangeView(-90.0,   0.3); }

		ImGui::EndMenu();
	}

	bool handle(const SDL_Event &event)
	{
		if (event.type != SDL_KEYDOWN) return false;

		auto key = event.key.keysym.sym;
		auto m   = event.key.keysym.mod;
		constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
		const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
		const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
		const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
		const int  mods  = shift + ctrl + alt;

		switch (key)
		{
			case SDLK_F1: case SDLK_F2:
			{
				int i = key - SDLK_F1; assert(i >= 0 && i < 2);
				if (mods == CTRL)
				{
					bool &b = (i == 0 ? gui.show_top_panel : gui.show_side_panel);
					if (gui.visible) b = !b;
					gui.show();
					return true;
				}
				break;
			}
		}
		return false;
	}
};
GUI_Menu *new_view_menu(GUI &gui) { return new GUI_ViewMenu(gui); }

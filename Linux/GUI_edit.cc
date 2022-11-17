#include "GUI.h"
#include "imgui/imgui.h"
#include "PlotWindow.h"

struct GUI_EditMenu : public GUI_Menu
{
	GUI_EditMenu(GUI &gui) : GUI_Menu(gui) {}

	void operator()()
	{
		if (!ImGui::BeginMenu("Edit")) return;
		
		std::string name("Undo"); bool on = gui.w.ut.can_undo(name);
		name += "###UndoItem";
		if (ImGui::MenuItem(name.c_str(), "CTRL+Z", false, on)) gui.w.ut.undo();

		name = "Redo"; on = gui.w.ut.can_redo(name);
		name += "###RedoItem";
		if (ImGui::MenuItem(name.c_str(), "CTRL+Y", false, on)) gui.w.ut.redo();

		ImGui::Separator();
		if (ImGui::MenuItem("Cut", "CTRL+X")) {}
		if (ImGui::MenuItem("Copy", "CTRL+C")) {}
		if (ImGui::MenuItem("Paste", "CTRL+V")) {}
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
			case SDLK_z:
				if (mods == CTRL) { gui.w.ut.undo(); gui.w.redraw(); return true; }
				break;
			case SDLK_y:
				if (mods == CTRL) { gui.w.ut.redo(); gui.w.redraw(); return true; }
				break;
		}
		return false;
	}
};
GUI_Menu *new_edit_menu(GUI &gui) { return new GUI_EditMenu(gui); }

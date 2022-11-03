#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include "PlotWindow.h"

void GUI::main_menu()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {}
			if (ImGui::MenuItem("Open", "Ctrl+O")) {}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {}
			if (ImGui::MenuItem("Save As..")) {}
			ImGui::Separator();

			if (ImGui::MenuItem("Checked", NULL, true)) {
			}
			if (ImGui::MenuItem("Quit", "Q")) {
				SDL_Event e; e.type = SDL_QUIT;
				SDL_PushEvent(&e);
			}
			ImGui::EndMenu();
		}
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
		if (ImGui::BeginMenu("View")) {
			#ifdef DEBUG
			ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
			#endif
			if (ImGui::MenuItem("Message Box"))
			{
				const SDL_MessageBoxButtonData buttons[] = {
				    {/* .flags, .buttonid, .text */ 0, 0, "no"},
				    {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1,
				     "yes"},
				    {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2,
				     "cancel"},
				};
				const SDL_MessageBoxColorScheme colorScheme = {
				    {/* .colors (.r, .g, .b) */
				     /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
				     {255, 0, 0},
				     /* [SDL_MESSAGEBOX_COLOR_TEXT] */
				     {0, 255, 0},
				     /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
				     {255, 255, 0},
				     /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND]
				      */
				     {0, 0, 255},
				     /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED]
				      */
				     {255, 0, 255}}};
				const SDL_MessageBoxData messageboxdata = {
				    SDL_MESSAGEBOX_INFORMATION, /* .flags */
				    NULL,			/* .window */
				    "example message box",	/* .title */
				    "select a button",		/* .message */
				    SDL_arraysize(buttons), /* .numbuttons */
				    buttons,		    /* .buttons */
				    &colorScheme	    /* .colorScheme */
				};
				int buttonid;
				if (SDL_ShowMessageBox(&messageboxdata,
						       &buttonid) < 0) {
					SDL_Log("error displaying message box");
					//return 1;
				}
				if (buttonid == -1) {
					SDL_Log("no selection");
				} else {
					SDL_Log("selection was %s",
						buttons[buttonid].text);
				}
				//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing file", "File is missing. Please reinstall the program.", NULL);
			}

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

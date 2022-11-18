#include "GUI.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include "PlotWindow.h"
#include "../Utility/Preferences.h"

extern volatile bool quit;
extern const std::vector<unsigned char> &font_data();
static std::string ini_location;

extern GUI_Menu *new_file_menu(GUI &gui);
extern GUI_Menu *new_edit_menu(GUI &gui);
extern GUI_Menu *new_graphs_menu(GUI &gui);
extern GUI_Menu *new_params_menu(GUI &gui);
extern GUI_Menu *new_defs_menu(GUI &gui);
extern GUI_Menu *new_view_menu(GUI &gui);

GUI::GUI(SDL_Window* window, SDL_GLContext context, PlotWindow &w)
: w(w)
, visible(false)
, need_redraw(1)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigInputTextCursorBlink = false;
	io.ConfigInputTrickleEventQueue = false;

	auto p = Preferences::directory();
	if (p.empty()) io.IniFilename = NULL; else {
		::ini_location = p / "imgui.ini";
		io.IniFilename = ::ini_location.c_str();
	}
	ImGui_ImplSDL2_InitForOpenGL(window, context);
	ImGui_ImplOpenGL2_Init();

	auto &fd = font_data();
	ImFontConfig fc; fc.FontDataOwnedByAtlas = false;
	static const ImWchar ranges[] = { 0x0001, 0xFFFF, 0 }; // get all
	io.Fonts->AddFontFromMemoryTTF((void *)fd.data(), (int)fd.size(), 18.0f, &fc, ranges);

	menus.emplace_back(new_file_menu(*this));
	menus.emplace_back(new_edit_menu(*this));
	menus.emplace_back(new_graphs_menu(*this));
	menus.emplace_back(new_params_menu(*this));
	menus.emplace_back(new_defs_menu(*this));
	menus.emplace_back(new_view_menu(*this));
}

GUI::~GUI()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void GUI::update()
{
	if (ImGui::GetIO().MouseDrawCursor != visible)
	{
		ImGui::GetIO().MouseDrawCursor = visible;

		if (!visible)
		{
			// if gui is visible, regular drawing will apply the cursor state,
			// but if not, draw one empty frame to update the state
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			ImGui::Render();
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
			ImGui::EndFrame();

			return;
		}
	}
	else if (!visible) return;

	bool dark = 
		w.plot.axis_type() == Axis::Invalid ||
		w.plot.axis.options.background_color.lightness() < 0.55;
	if (dark) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::GetStyle().FrameBorderSize = dark ? 0.0f : 1.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::BeginMainMenuBar())
	{
		for (auto &m : menus) (*m)();
		ImGui::EndMainMenuBar();
	}

	top_panel_height = 0.0f;
	if (show_top_panel) top_panel();
	if (show_side_panel) side_panel();
	ImGui::PopStyleVar(); // draw other windows with border

	error_panel();
	confirmation_panel();
	prefs_panel();
	help_panel();

	#ifdef DEBUG
	if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
	#endif
}

void GUI::draw()
{
	if (!visible) return;
	ImGui::Render();
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	if (need_redraw > 0) --need_redraw;
}

void GUI::error(const std::string &msg)
{
	error_msg = msg;
	visible = true;
	redraw();
	// cannot call OpenPopup here! would cause the IDs to mismatch.
}

void GUI::confirm(bool c, const std::string &msg, std::function<void(void)> action)
{
	assert(action);
	if (!c)
	{
		try
		{
			action();
		}
		catch (std::exception &e)
		{
			error(e.what());
		}
		return;
	}
	confirm_msg = msg;
	confirm_action = action;
	visible = true;
	redraw();
}

bool GUI::handle_event(const SDL_Event &event)
{
	bool h = ImGui_ImplSDL2_ProcessEvent(&event);
	if (visible && h) redraw();
	ImGuiIO &io = ImGui::GetIO();

	bool activate = false, handled = false;

	for (auto &m : menus) if (m->handle(event)) { handled = true; break; }

	if (handled)
	{}
	else if (event.type == SDL_QUIT)
	{
		confirm(w.ut.have_changes(), "Document has unsaved changes. Continue?",
		[]{ quit = true; });
		return true;
	}
	else if (event.type == SDL_KEYUP)
	{
		// workaround stuck modifier keys in imgui's SDL backend
		auto key = event.key.keysym.sym;
		switch (key)
		{
			case SDLK_LCTRL:
			case SDLK_RCTRL:
			{
				ImGuiIO& io = ImGui::GetIO();
    				io.AddKeyEvent(ImGuiMod_Ctrl, false);
				break;
			}
			case SDLK_LALT:
			case SDLK_RALT:
			{
				ImGuiIO& io = ImGui::GetIO();
    				io.AddKeyEvent(ImGuiMod_Alt, false);
				break;
			}
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
			{
				ImGuiIO& io = ImGui::GetIO();
    				io.AddKeyEvent(ImGuiMod_Shift, false);
				break;
			}
		}
	}
	else if (event.type == SDL_KEYDOWN)
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
			case SDLK_l:
				if (mods == CTRL)
				{
					ImGui::SetWindowFocus("Graph Definition");
					// TODO: focus the input field
					activate = handled = true;
				}
				break;
			case SDLK_TAB:
			case SDLK_RETURN:
				if (mods || visible) break;
				activate = handled = true;
				break;
			case SDLK_F1: case SDLK_F2: case SDLK_F3: case SDLK_F4:
			case SDLK_F5: case SDLK_F6: case SDLK_F7: case SDLK_F8:
			case SDLK_F9: case SDLK_F10: case SDLK_F11: case SDLK_F12:
			{
				int i = key - SDLK_F1; assert(i >= 0 && i < 12);
				if (mods == 0)
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
				if (mods || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId + ImGuiPopupFlags_AnyPopupLevel))
					break;
				visible = !visible;
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

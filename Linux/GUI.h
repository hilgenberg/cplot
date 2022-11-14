#pragma once
#include <SDL.h>
#include "imgui/imgui.h"
#include "../Engine/cnum.h"
#include "../Engine/Namespace/ObjectDB.h"
class PlotWindow;
class Parameter;

class GUI
{
public:
	GUI(SDL_Window* window, SDL_GLContext context, PlotWindow &w);
	~GUI();

	bool handle_event(const SDL_Event &event); // GUI_menu.cc
	bool needs_redraw() const{ return visible && need_redraw > 0; }

	operator bool() const{ return visible; }
	void toggle() { visible = !visible; redraw(); }
	void close() { visible = false; }

	void update();
	void draw();
	void redraw(int n_frames = 3){ need_redraw = std::max(n_frames, need_redraw); }

private:
	PlotWindow &w;
	bool visible;
	int  need_redraw;

	#ifdef DEBUG
	bool show_demo_window = false;
	#endif
	
	void  main_menu(), file_menu(), graphs_menu(), params_menu(), defs_menu(), settings_menu();
	void  main_panel(), settings_panel();
	bool  show_main_panel = true, show_settings_panel = true;
	void  prefs_panel(); bool show_prefs_panel = false;

	float main_panel_height = 0.0f;
	
	// Parameter editor and its data
	void param_editor();
	bool param_edit = false;
	IDCarrier::OID param_orig = 0;
	std::string param_tmp[7];
	int param_tmp_type;
	bool param_tmp_rad;

	// UserFunction editor and its data
	void def_editor();
	bool def_edit = false;
	IDCarrier::OID def_orig = 0;
	std::string def_tmp;

	// manage ImGui's BeginDisabled/EndDisabled regions
	bool enabled = true;
	void enable(bool e = true);

	// printing and parsing helpers
	std::string format_double(double x) const;
	double parse(const std::string &s, const char *desc);
	std::string format_complex(cnum x) const;
	cnum cparse(const std::string &s, const char *desc);

	// error and confirmation messages
	void error(const std::string &msg);
	void error_panel();
	std::string error_msg;

	void confirm(bool need_confirmation, const std::string &msg, std::function<void(void)> action);
	void confirmation_panel();
	std::string confirm_msg;
	std::function<void(void)> confirm_action;

	//--- actions with hotkeys ------------------------------

	void open_file();
	void save_as();
	void save();

};

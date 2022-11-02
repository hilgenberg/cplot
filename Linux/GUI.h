#pragma once
#include <SDL.h>
#include "imgui/imgui.h"
class PlotWindow;

class GUI
{
public:
	GUI(SDL_Window* window, SDL_GLContext context, PlotWindow &w);
	~GUI();

	bool handle_event(const SDL_Event &event);
	bool needs_redraw() const{ return visible && need_redraw > 0; }

	operator bool() const{ return visible; }
	void toggle() { visible = !visible; redraw(); }
	void close() { visible = false; }

	void update();
	void draw();
	void redraw(){ need_redraw = 3; }

private:
	PlotWindow &w;
	bool visible;
	int  need_redraw;
	
	bool enabled;
	bool enable(bool e);
	void begin_section();
	void end_section();
	
	void settings_window();
};

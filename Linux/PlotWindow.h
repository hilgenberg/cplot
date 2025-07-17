#pragma once
#include "Document.h"
#include "../Graphs/OpenGL/GL_RM.h"
#include "../Graphs/OpenGL/GL_StringCache.h"
#include "../Engine/Namespace/Parameter.h"
#include "../Utility/FPSCounter.h"
#include <map>
#include <SDL_events.h>

class PlotWindow : public Document
{
public:
	PlotWindow(SDL_Window* window, GL_Context &context);
	virtual ~PlotWindow();
	void load(const std::string &path) override { Document::load(path); redraw(); }
	void load_default() override { Document::load_default(); redraw(); }

	bool needs_redraw() const{ return need_redraw; }
	bool animating() const{ return tnf > 0.0; }

	void animate();
	void draw();
	void waiting() { fps.pause(); }
	//void redraw(){ need_redraw = true; } --> moved to base class

	void reshape(int w, int h);
	bool handle_event(const SDL_Event &event);
	bool handle_key(SDL_Keysym key, bool release);

	int  accum_size() const { return accum; }
	float status_height() const { return current_status_height; }
	
	void start_animations(); // call after Parameter::anim_start()

protected:
	SDL_Window *window;
	int         w, h;
	int         accum;      // accumulation buffer size
   
	double      tnf;        // scheduled time for next frame
	double      last_frame; // time of last animate() call
	GL_RM       rm;
	FPSCounter  fps;

	std::map<SDL_Keycode, double> ikeys; // pressed key -> inertia
	std::set<SDL_Keycode> keys; // pressed keys

	enum Zoom{ Axis, Camera, Inrange }; 
	void translate(double dx, double dy, double dz, Zoom what, int mx = -1, int my = -1);
	void change_parameter(int i, cnum delta);

	void status(); // draw status bar
	float current_status_height = 0.0f;
	mutable GL_StringCache labelCache;
	struct StatusField { double x0, x1, y0, y1; };
	std::vector<StatusField> status_fields; // for matching clicks in status bar to parameters
	int dragged_param = 0;
};


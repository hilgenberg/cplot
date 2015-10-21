#pragma once
#include "XWindow.h"
#include "Document.h"
#include "../Graphs/OpenGL/GL_RM.h"
#include "../Graphs/OpenGL/GL_StringCache.h"
#include "../Engine/Namespace/Parameter.h"
#include <map>

class PlotWindow : public XWindow, public Document
{
public:
	PlotWindow();
	virtual ~PlotWindow();

	bool   animating() const{ return tnf > 0.0; }
	double next_frame_schedule() const{ return tnf; }
	void   animate(double now);
	bool   needs_redraw() const{ return need_redraw; }

	void draw();
	void redraw(){ need_redraw = true; }

	virtual void load(const std::string &path){ Document::load(path); redraw(); }

	virtual void reshape();
	virtual bool handle_key(KeySym key, const char *s, bool release);
	virtual bool handle_scroll(double dx, double dy, bool discrete);
	virtual bool handle_drag(int buttons, double dx, double dy);
	virtual bool handle_other(XEvent &e);

	enum AnimType{ Linear, Saw, PingPong, Sine };
	void animate(Parameter &p, const cnum &v0, const cnum &v1, double dt, int reps=-1, AnimType type=Sine);
	void stop_animations(){ panims.clear(); }
	void stop_animation(Parameter &p){ panims.erase(p.oid()); }

	bool focus(Window w);

protected:
	void start(); // animating
	void stop();

	double   tnf; // scheduled time for next frame
	double   last_frame; // time of last draw
	bool     need_redraw; // call draw after all pending events are handled
	GL_RM    rm;

	struct ParameterAnimation
	{
		double dt, t0;
		cnum v0, v1;
		AnimType type;
		int reps; // -1 for forever
	};
	std::map<IDCarrier::OID, ParameterAnimation> panims;

	std::map<KeySym, double> ikeys; // pressed key -> inertia
	std::set<KeySym> keys; // pressed keys

	void move(double dx, double dy, double dz, bool kbd, int buttons=0);
	enum Zoom{ Axis, Camera, Inrange }; 
	void zoom(double d, Zoom what, int mx = -1, int my = -1);
	void change_parameter(int i, cnum delta);

	void status(); // draw status bar
	mutable GL_StringCache labelCache;
};


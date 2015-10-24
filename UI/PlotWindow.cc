#include "PlotWindow.h"
#include "../Utility/StringFormatting.h"
#include "../Utility/System.h"
#include "../Graphs/Plot.h"
#include "../Graphs/Geometry/Axis.h"
#include "../Graphs/Geometry/Camera.h"
#include "XWindow.h"
#include "Document.h"
#include <vector>
#include <set>
#include <algorithm>
#include "../Engine/Namespace/all.h"
#include "main.h"

#include <cassert>
#include <X11/extensions/XI2.h>

#define ScrollUpButton    5
#define ScrollDownButton  4
#define ScrollLeftButton  7
#define ScrollRightButton 6

#define FPS 90

PlotWindow::PlotWindow()
: XWindow(format("%s", arg0))
, tnf(-1.0)
, last_frame(-1.0)
, rm(GL_Context(context))
{
}
PlotWindow::~PlotWindow(){ }

void PlotWindow::stop(){ tnf = -1.0; }
void PlotWindow::start(){ if (tnf <= 0.0) tnf = now() + 1.0/FPS; }

void PlotWindow::animate(double t)
{
	assert(tnf > 0.0 && t >= tnf);
	const double spf = 1.0/FPS;
	double dt = std::min(std::max(1.0, (t - last_frame)/spf), 5.0);
	double dx = 0.0, dy = 0.0, dz = 0.0;
	int idx = 0, idy = 0, idz = 0;
	std::set<int> params;

	for (auto &i : ikeys)
	{
		double &inertia = i.second;
		inertia += (0.03 + inertia*0.04)*dt;
		inertia = std::min(inertia, 5.0);
		
		switch (i.first)
		{
			case XK_Left:  --idx; dx -= inertia; break;
			case XK_Right: ++idx; dx += inertia; break;
			case XK_Up:    --idy; dy -= inertia; break;
			case XK_Down:  ++idy; dy += inertia; break;
			case XK_plus:  ++idz; dz += inertia; break;
			case XK_minus: --idz; dz -= inertia; break;
			default: assert(false); break;
		}
	}
	for (auto k : keys)
	{
		switch (k)
		{
			case XK_0: params.insert(9); break;
			case XK_1: case XK_2: case XK_3: case XK_4:
			case XK_5: case XK_6: case XK_7: case XK_8:
			case XK_9: params.insert(k - XK_1); break;
			default: assert(false); break;
		}
	}

	if (!params.empty())
	{
		for (int i : params)
		{
			change_parameter(i, cnum(idx, -idy));
		}
	}
	else
	{
		move(dx*dt, dy*dt, dz*dt, true);
	}

	for (auto i = panims.begin(); i != panims.end(); )
	{
		Parameter *P = (Parameter*)IDCarrier::find(i->first);
		if (!P)
		{
			// Parameter was deleted
			panims.erase(i++);
			continue;
		}
		auto &a = i->second;
		double r, p = modf((t-a.t0)/a.dt, &r);
		if (a.reps > 0 && r >= a.reps)
		{
			// finished
			P->value((a.type == Saw || a.reps & 1) ? a.v1 : 
			          a.type == Linear ? a.v0+(a.v1-a.v0)*(double)a.reps :
			          a.v0);
			panims.erase(i++);
			continue;
		}

		switch (a.type)
		{
			case Linear: p += r; break;
			case Saw: break;
			case Sine:
				p += r; p /= 2.0; p = modf(p, &r);
				p = 0.5 - 0.5*cos(2.0*M_PI*p); break;
			case PingPong:
				p += r; p /= 2.0; p = modf(p, &r);
				p *= 2.0; if (p > 1.0) p = 2.0-p; break;
		}
		cnum v = a.v1*p + a.v0*(1.0-p);
		if (P->type() == Integer)
		{
			// we don't want rounding here
			bool f = (a.v0.real() < a.v1.real());
			v = f ? floor(v.real()) : ceil(v.real());
		}
		P->value(a.v1*p + a.v0*(1.0-p));
		plot.recalc(P);
		++i;
	}

	draw();

	if (panims.empty() && ikeys.empty() && plot.axis_type() != Axis::Invalid)
	{
		stop();
		return;
	}

	double t1 = std::max(now(), t + spf);
	while (tnf + spf*0.01 < t1) tnf += spf;
}

//static inline void toggle(bool &what){ what = !what; }

bool PlotWindow::handle_key(KeySym key, const char *s, bool release)
{
	if (!release)
	{
		switch (key)
		{
			case XK_Left: case XK_Right:
			case XK_Up:   case XK_Down:
			case XK_plus: case XK_minus:
				if (!ikeys.count(key)) ikeys[key] = 0.0;
				start();
				return true;
		
			case XK_0: case XK_1: case XK_2: case XK_3: case XK_4:
			case XK_5: case XK_6: case XK_7: case XK_8: case XK_9:
				if (!keys.count(key)) redraw();
				keys.insert(key);
				return true;
		}

		int len = strlen(s);
		if (len != 1) return false;
		
		auto toggle = [this](bool &what){ what = !what; redraw(); return true; };
		auto view   = [this](double x, double y)
		{
			switch (plot.axis_type())
			{
				case Axis::Invalid:
				case Axis::Rect: return false;
				default: plot.camera.set_angles(x, y, 0.0); redraw(); return true;
			}
		};

		Graph *g = plot.current_graph();
		switch (s[0])
		{
			case 'q': case 'Q': case 3: case 27: closed = true; return true;
			case '.': stop_animations(); return true;

			case 'a': return toggle(plot.axis.options.hidden);
			case 'c': if (g && g->toggle_clipping()) redraw(); return true;
			case 'g': if (g && g->toggle_grid(false)) redraw(); return true;
			case 'G': if (g && g->toggle_grid(true)) redraw(); return true;

			case 'e': plot.axis.equal_ranges(); plot.recalc(); redraw(); return true;
			case 't': view(  0,  90); return true;
			case 'T': view(  0, -90); return true;
			case 'f': view(  0,   0); return true;
			case 'F': view(180,   0); return true;
			case 's': view(-90,   0); return true;
			case 'S': view( 90,   0); return true;
			case 'z': plot.axis.reset_center(); plot.recalc(); redraw(); return true;

			/*
			case 'd': [settingsBox toggle:settingsBox.disco];      return;
			case 'C': [settingsBox toggle:settingsBox.clipCustom]; return;
			case 'l': [settingsBox toggle:settingsBox.clipLock];   return;
			case 'L': [settingsBox   push:settingsBox.clipReset];  return;
				
			case 't': [settingsBox  cycle:settingsBox.textureMode direction:+1]; return;
			case 'T': [settingsBox  cycle:settingsBox.textureMode direction:-1]; return;
			case 'v': prop(CYCLE, P_VF_MODE); return;
			case 'V': [settingsBox  cycle:settingsBox.vfMode      direction:-1]; return;

			case 'e': [axisBox equalRanges]; return;
			case 'z': [axisBox  centerAxis]; return;*/

			case '\r':
			case '\t':
			case '\n': if (terminalID) focus(terminalID); break;
		}
	}
	else
	{
		switch (key)
		{
			case XK_Left: case XK_Right:
			case XK_Up:   case XK_Down:
			case XK_plus: case XK_minus:
				ikeys.erase(key);
				return true;
		
			case XK_0: case XK_1: case XK_2: case XK_3: case XK_4:
			case XK_5: case XK_6: case XK_7: case XK_8: case XK_9:
				if (keys.count(key)) redraw();
				keys.erase(key);
				return true;
		}
	}
	return false;
}

bool PlotWindow::focus(Window w)
{
	XWindowAttributes a;
	if (!XGetWindowAttributes(display, w, &a)) return false;
	if (a.map_state != IsViewable) return false;
	XSetInputFocus(display, w, RevertToParent, CurrentTime);
	XMapRaised(display, w);
	return true;
}

bool PlotWindow::handle_scroll(double dx, double dy, bool discrete)
{
	double s = discrete ? 5.0 : 1.0;
	move(s*dx, s*dy, 0, false, 0);
	return true;
}

bool PlotWindow::handle_drag(int buttons, double dx, double dy)
{
	move(dx, dy, 0, false, buttons);
	return true;
}

bool PlotWindow::handle_other(XEvent &e)
{
	int t = e.type;
	if (t == Expose || t == GraphicsExpose || t == VisibilityNotify || t == CreateNotify || t == MapNotify || t == ReparentNotify || t == ConfigureNotify)
	{
		redraw();
		return true;
	}
	return false;
}
	
void PlotWindow::reshape()
{
	glViewport(0, 0, w, h);
	plot.camera.viewport(w, h, plot.axis);
	plot.update_axis();
	plot.update(CH_UNKNOWN);
	redraw();
}


void PlotWindow::draw()
{
	if (last_frame <= 0.0) reshape();
	last_frame = now();

	if (plot.axis_type() == Axis::Invalid)
	{
		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		double hr = plot.camera.aspect();
		glOrtho(-1.0, 1.0, -hr, hr, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW); glLoadIdentity();

		float y = 0.0f;
		glClearColor(y, y, y, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);

		double r = std::min(hr, 1.0/hr)*0.5, x0=0.0, y0=0.0;
		y = 0.035f;
		glColor4f(y, y, y, 1.0f);
		auto tri = [&x0,&y0](double r, double a){ for (int i=0; i<3; ++i, a += 2.0*M_PI/3.0) glVertex2d(x0+r*cos(a), y0+r*sin(a)); };
		glBegin(GL_TRIANGLES);
		double a = last_frame*2.0*M_PI / 12.0;
		tri(r,a);
		a += M_PI;
		for (int i = 0; i < 3; ++i, a += 2.0*M_PI/3.0)
		{
			x0 = r*cos(a); y0 = r*sin(a);
			tri(r*0.5,-0.75*a);
		}
		glEnd();

		swap_buffers();
		need_redraw = false;
		start();
		return;
	}

	bool dynamic = true; // todo: from pref
	bool anim = dynamic && (!ikeys.empty() || modifiers & (Button1Mask|Button2Mask|Button3Mask));
	if (!anim && !plot.at_full_quality()) plot.update(CH_UNKNOWN);
	GL_CHECK;

	int nt = -1; // todo: pref
	if (nt < 1 || nt > 256) nt = n_cores;
	plot.draw(rm, nt, accum, anim);
	GL_CHECK;

	status();
	GL_CHECK;

	swap_buffers();
	need_redraw = !plot.at_full_quality();
}

static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }

void PlotWindow::move(double dx, double dy, double dz, bool kbd, int buttons)
{
	//if (dx*dx+dy*dy+dz*dz < 1.0) return;
	auto axis = plot.axis.type();
	if (axis == Axis::Invalid) return;
	double pixel = plot.pixel_size();

	//fprintf(stderr, "move %g %g %g %d %d\n", dx, dy, dz, (int)kbd, buttons);
	const int SHIFT=1, CTRL=2, ALT=4;
	int shift  = (modifiers & ShiftMask) ? SHIFT : 0;
	int ctrl   = (modifiers & ControlMask) ? CTRL : 0;
	int alt    = (modifiers & (Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)) ? ALT : 0;
	int mods   = shift + ctrl + alt;
	bool b0    = !kbd && !buttons; // scrollwheel or trackpad
	bool b1    = buttons & Button1Mask;
	bool b2    = buttons & Button2Mask;
	bool b3    = buttons & Button3Mask;

	(void)b2; (void)b3;

	if (axis == Axis::Rect)
	{
		if (kbd || b1 || b0)
		{
			if (!mods)
			{
				// move (dx,dy), zoom dz
				dx *= pixel;
				dy *= pixel;
				plot.axis.move(-dx, dy, 0.0);
				plot.update(CH_AXIS_RANGE);
				zoom(dz, Axis);
				redraw();
			}
			else if (mods == SHIFT)
			{
				zoom(0.5*absmax(dx, dy), Axis);
			}
			else if (mods == ALT+SHIFT)
			{
				plot.axis.in_zoom(exp(absmax(dx, dy) * 0.01));
				plot.update(CH_IN_RANGE);
			}
			else if (mods == ALT)
			{
				dx *= pixel;
				dy *= pixel;
				plot.axis.in_move(dx, -dy);
				plot.update(CH_IN_RANGE);
			}
		}
	}
	else
	{
		if (kbd || b1 || b0)
		{
			if (!mods)
			{
				plot.camera.rotate(0.01 * dy, 0, 0.01 * dx);
				zoom(dz, Camera);
				redraw();
			}
			else if (mods == SHIFT)
			{
				zoom(0.5*absmax(dx,dy), Camera);
			}
			else if (mods == SHIFT+CTRL)
			{
				zoom(0.5*absmax(dx,dy), Axis);
			}
			else if (mods == CTRL)
			{
				float f;
				plot.camera.scalefactor(0, f);
				dx /= f; dy /= f;
				plot.camera.move(plot.axis, -dx*pixel, dy*pixel, 0);
				plot.update(CH_AXIS_RANGE);
				redraw();
			}
			else if (mods == ALT+SHIFT)
			{
				zoom(-0.5*absmax(dx, dy), Inrange);
			}
			else if (mods == ALT)
			{
				plot.axis.in_move(dx*0.01, -dy*0.01);
				plot.update(CH_IN_RANGE);
				redraw();
			}
		}
	}
}
void PlotWindow::zoom(double d, PlotWindow::Zoom what, int mx, int my)
{
	if (fabs(d) < 0.001) return;
	double f = exp(-d * 0.02);
	redraw();
	
	if (plot.axis.type() == Axis::Rect)
	{
		if (what == Inrange)
		{
			plot.axis.in_zoom(1.0/f);
			plot.update(CH_IN_RANGE);
		}
		else
		{
			auto &axis = plot.axis;
			double x0 = 2.0 * mx / w - 1.0; // [-1,1]
			double y0 = 2.0 * my / h - 1.0;
			double x1 = (x0 * axis.range(0)) + axis.center(0);
			double y1 = (y0 * axis.range(1)) + axis.center(1);
			axis.zoom(f);
			if (mx >= 0 && my >= 0)
			{
				double x2 = (x0 * axis.range(0)) + axis.center(0);
				double y2 = (y0 * axis.range(1)) + axis.center(1);
				axis.move(x1-x2, y1-y2, 0.0);
			}
			plot.update(CH_AXIS_RANGE);
		}
	}
	else
	{
		switch (what)
		{
			case Inrange:
				plot.axis.in_zoom(1.0/f);
				plot.update(CH_IN_RANGE);
				break;
			case Axis:
				plot.axis.zoom(f);
				plot.update(CH_AXIS_RANGE);
				break;
			case Camera:
				plot.camera.zoom(f);
				break;
		}
	}
}

void PlotWindow::change_parameter(int i, cnum delta)
{
	if (i < 0) return;
	std::set<Parameter*> aps(plot.used_parameters());
	if ((size_t)i >= aps.size()) return;
	std::vector<Parameter*> ps(aps.begin(), aps.end());
	std::sort(ps.begin(), ps.end(), [&](Parameter *a, Parameter *b)->bool{ return a->name() < b->name(); });
	Parameter *p = ps[i];

	static double last_param_change = -1.0;
	switch (p->type())
	{
		case Angle:
		case ComplexAngle:
			if (!p->angle_in_radians()) delta /= DEGREES;
			p->rvalue(p->rvalue() + 0.1*delta.real()); break;

		case Integer:
			if (last_param_change > 0.0 && now() - last_param_change < 0.25) return;
			p->rvalue(p->rvalue() + delta.real());
			last_param_change = now();
			break;

		case Real:
			if (defined(p->min()) && defined(p->max())) delta *= (p->max()-p->min())*0.4;
			p->value(p->value() + delta * 0.1);
			break;
			
		case Complex:
			if (defined(p->rmax())) delta *= 2.0*p->rmax()*0.4;
			else if (defined(p->min()) && defined(p->max())) delta *= (p->max()-p->min())*0.4;
			else if (defined(p->imin()) && defined(p->imax())) delta *= (p->imax()-p->imin())*0.4;
			p->value(p->value() + delta * 0.1);
			break;
	}
	
	if (plot.recalc(p)) redraw();
}

void PlotWindow::status()
{
	std::set<Parameter*> aps(plot.used_parameters());
	if (aps.empty()) return;
	std::vector<Parameter*> ps(aps.begin(), aps.end());
	std::sort(ps.begin(), ps.end(), [&](Parameter *a, Parameter *b)->bool{ return a->name() < b->name(); });

	bool dark = plot.axis.options.background_color.lightness() < 0.3;
	double fs = 20.0f;
	std::vector<GL_Font> fonts;
	fonts.push_back(GL_Font("sans regular", fs));
	fonts.push_back(fonts[0]);
	fonts[0].color = GL_Color(dark ? 0.5f : 0.3f);
	fonts[1].color = GL_Color(1.0f, 0.0f, 0.0f);
	
	labelCache.fonts(std::move(fonts));
	labelCache.start();

	std::vector<GL_String*> labels;
	double vspace = fs/4.0, hspace = fs/2.0;
	double lh = 0.0, x = hspace;
	int rows = 1, i = 0;
	for (Parameter *p : ps)
	{
		bool sel = (i < 10 && keys.count(i==9 ? XK_0 : XK_1+i));
		labelCache.font(sel ? 1 : 0);
		GL_String *s = labelCache.get(format("%s = %s", p->name().c_str(), to_string(p->value(), rns).c_str()));
		labels.push_back(s);
		if (s->h() > lh) lh = s->h();
		if (x > hspace && x + s->w() + hspace > w){ x = hspace; ++rows; }
		x += s->w() + 2.0*hspace;
		++i;
	}

	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
	glOrtho(0, w, h, 0, -1.0, 0.1);

	float c = dark ? 0.0f :1.0f;
	glColor4d(c, c, c, dark ? 0.85 : 0.7);
	glBegin(GL_QUADS);
	glVertex2d(0, 0);
	glVertex2d(w, 0);
	glVertex2d(w, rows*lh+vspace*(rows+1));
	glVertex2d(0, rows*lh+vspace*(rows+1));
	glEnd();

	x = hspace; double y = vspace;
	for (GL_String *s : labels)
	{
		if (x > hspace && x + s->w() + hspace > w){ x = hspace; y += lh + vspace; }
		s->draw2d(x, y, s->w(), s->h());
		x += s->w() + 2.0*hspace;
	}
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);  glPopMatrix();
	labelCache.finish();
}

void PlotWindow::animate(Parameter &p, const cnum &v0, const cnum &v1, double dt, int reps, AnimType type)
{
	if (dt < 0.0) return;
	ParameterAnimation &a = panims[p.oid()];
	a.v0 = v0; a.v1 = v1; a.dt = dt;
	a.reps = reps;
	a.type = type;
	a.t0 = now();
	start();
}


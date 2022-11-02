#include "PlotWindow.h"
#include "../Utility/StringFormatting.h"
#include "../Utility/System.h"
#include "../Graphs/Plot.h"
#include "../Graphs/Geometry/Axis.h"
#include "../Graphs/Geometry/Camera.h"
#include "Document.h"
#include <vector>
#include <set>
#include <algorithm>
#include "../Engine/Namespace/all.h"
#include <cassert>

#define ScrollUpButton    5
#define ScrollDownButton  4
#define ScrollLeftButton  7
#define ScrollRightButton 6

#define FPS 90

PlotWindow::PlotWindow(SDL_Window* window, GL_Context &context)
: tnf(-1.0)
, last_frame(-1.0)
, rm(context)
, accum(0)
, w(0), h(0)
, window(window)
, closed(false)
{
	int n = 0;
	SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE, &accum);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &n); if (n < accum) accum = n;
	SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE, &n); if (n < accum) accum = n;
	SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &n); if (n < accum) accum = n;
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
			case SDLK_LEFT:  --idx; dx -= inertia; break;
			case SDLK_RIGHT: ++idx; dx += inertia; break;
			case SDLK_UP:    --idy; dy -= inertia; break;
			case SDLK_DOWN:  ++idy; dy += inertia; break;
			case SDLK_PLUS:  ++idz; dz += inertia; break;
			case SDLK_MINUS: --idz; dz -= inertia; break;
			default: assert(false); break;
		}
	}
	for (auto k : keys)
	{
		switch (k)
		{
			case SDLK_0: params.insert(9); break;
			case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
			case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8:
			case SDLK_9: params.insert(k - SDLK_1); break;
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

bool PlotWindow::handle_event(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_QUIT: closed = true; return true;
		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
				case SDL_WINDOWEVENT_CLOSE:
					if (e.window.windowID == SDL_GetWindowID(window))
					{
						closed = true;
						return true;
					}
					break;
				case SDL_WINDOWEVENT_SHOWN:
				case SDL_WINDOWEVENT_EXPOSED:
				case SDL_WINDOWEVENT_RESTORED:
					redraw();
					return true;
			}
			return false;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return handle_key(e.key.keysym, e.type == SDL_KEYUP);

		case SDL_MOUSEMOTION:
		{
			auto buttons = e.motion.state;
			if (!(buttons & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK)))
				return true;
			int dx = e.motion.xrel, dy = e.motion.yrel;
			if (dx*dx + dy*dy < 4) return true;
			move(dx, dy, 0.0, false, buttons);
			return true;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			return true;
		case SDL_MOUSEWHEEL:
		{
			//printf("SCRL %g %g - %d %d - %d\n", e.wheel.preciseX, e.wheel.preciseY, e.wheel.x, e.wheel.y, e.wheel.which);
			if (e.wheel.preciseX != e.wheel.x || e.wheel.preciseY != e.wheel.y)
			{
				move(-5.0*e.wheel.preciseX, 5.0*e.wheel.preciseY, 0, false, 0);
			}
			else
			{
				move(0, 0, -5.0*(e.wheel.x + e.wheel.y), false, 0);
			}
			return true;
		}
		case SDL_MULTIGESTURE:
		{
			// UNTESTED: my trackpad does not send these...
			auto &g = e.mgesture;
			printf("MULT %g %g - %g %g\n", g.x, g.y, g.dDist, g.dTheta);
			move(0, 0, -5.0*e.mgesture.dDist, false, 0);
			return true;
		}
	}
	return false;
}

bool PlotWindow::handle_key(SDL_Keysym keysym, bool release)
{
	auto key = keysym.sym;
	bool shift = keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT);
	bool control = keysym.mod & (KMOD_LCTRL|KMOD_RCTRL);
	auto view   = [this](double x, double y)
	{
		switch (plot.axis_type())
		{
			case Axis::Invalid:
			case Axis::Rect: return false;
			default: plot.camera.set_angles(x, y, 0.0); redraw(); return true;
		}
	};

	if (!release) switch (key)
	{
		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			if (!ikeys.count(key)) ikeys[key] = 0.0;
			start();
			return true;
	
		case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
		case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
			if (!keys.count(key)) redraw();
			keys.insert(key);
			return true;
	
		case SDLK_q: closed = true; return true;
		case SDLK_PERIOD: stop_animations(); return true;

		case SDLK_a: return toggleAxis();
		case SDLK_c: return toggleClip();
		case SDLK_g: return toggleGrid();

		case SDLK_e: plot.axis.equal_ranges(); plot.recalc(); redraw(); return true;
		case SDLK_t: view(  0, shift ? -90 : 90); return true;
		case SDLK_f: view(shift ? 180 : 0,   0); return true;
		case SDLK_s: view(shift ? 90 : -90,   0); return true;
		
		case SDLK_z:
			if (control)
			{
				ut.undo();
				return true;
			}
			else
			{
				plot.axis.reset_center(); plot.recalc(); redraw(); return true;
			}
		case SDLK_y:
			if (control)
			{
				ut.redo();
				return true;
			}
			break;

		case SDLK_v: return cycleVFMode(shift ? -1 : 1);

		/*
		case SDLK_d: [settingsBox toggle:settingsBox.disco];      return;
		case SDLK_C: [settingsBox toggle:settingsBox.clipCustom]; return;
		case SDLK_l: [settingsBox toggle:settingsBox.clipLock];   return;
		case SDLK_L: [settingsBox   push:settingsBox.clipReset];  return;
			
		case SDLK_t: [settingsBox  cycle:settingsBox.textureMode direction:+1]; return;
		case SDLK_T: [settingsBox  cycle:settingsBox.textureMode direction:-1]; return;

		case SDLK_e: [axisBox equalRanges]; return;
		case SDLK_z: [axisBox  centerAxis]; return;*/
	}
	else switch (key)
	{
		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			ikeys.erase(key);
			return true;
	
		case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
		case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
			if (keys.count(key)) redraw();
			keys.erase(key);
			return true;
	}
	return false;
}

void PlotWindow::reshape(int w_, int h_)
{
	if (w == w_ && h == h_) return;
	w = w_; h = h_;
	if (w <= 0 || h <= 0) return;
	glViewport(0, 0, w, h);
	plot.camera.viewport(w, h, plot.axis);
	plot.update_axis();
	plot.update(CH_UNKNOWN);
	redraw();
}


void PlotWindow::draw()
{
	//if (last_frame <= 0.0) reshape();
	last_frame = now();
	if (w <= 0 || h <= 0) return;

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

		need_redraw = false;
		start();
		return;
	}
	

	bool dynamic = true; // todo: from pref
	bool anim = dynamic && (!ikeys.empty() || SDL_GetMouseState(NULL,NULL) & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK));
	if (!anim && !plot.at_full_quality()) plot.update(CH_UNKNOWN);
	GL_CHECK;

	int nt = -1; // todo: pref
	if (nt < 1 || nt > 256) nt = n_cores;
	plot.draw(rm, nt, accum, anim);
	GL_CHECK;

	status();
	GL_CHECK;

	need_redraw = !plot.at_full_quality();
}

static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }

void PlotWindow::move(double dx, double dy, double dz, bool kbd, int buttons)
{
	//if (dx*dx+dy*dy+dz*dz < 1.0) return;
	auto axis = plot.axis.type();
	if (axis == Axis::Invalid) return;
	double pixel = plot.pixel_size();

	SDL_Keymod m = SDL_GetModState();
	//fprintf(stderr, "move %g %g %g %d %d\n", dx, dy, dz, (int)kbd, buttons);
	const int SHIFT=1, CTRL=2, ALT=4;
	int  shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
	int  ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
	int  alt     = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0; // alt|altGr
	int  mods    = shift + ctrl + alt;
	bool b0      = !kbd && !buttons; // scrollwheel or trackpad
	bool b1      = buttons & SDL_BUTTON_LMASK;
	bool b2      = buttons & SDL_BUTTON_RMASK;
	bool b3      = buttons & SDL_BUTTON_MMASK;
	
	#if 0
	#define M(i) (modifiers & Mod ## i ## Mask)
	fprintf(stderr, "mods: %d %d %d %d %d\n", M(1), M(2), M(3), M(4), M(5));
	#undef M
	#endif
	
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
		bool sel = (i < 10 && keys.count(i==9 ? SDLK_0 : SDLK_1+i));
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


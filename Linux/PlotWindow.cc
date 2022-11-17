#include "PlotWindow.h"
#include "Document.h"
#include "../Utility/StringFormatting.h"
#include "../Utility/Preferences.h"
#include "../Graphs/Plot.h"
#include "../Graphs/Geometry/Axis.h"
#include "../Graphs/Geometry/Camera.h"
#include "../Engine/Namespace/all.h"
extern volatile bool quit;

#define ScrollUpButton    5
#define ScrollDownButton  4
#define ScrollLeftButton  7
#define ScrollRightButton 6

#define FPS 90

static inline double absmax(double a, double b){ return fabs(a) > fabs(b) ? a : b; }

PlotWindow::PlotWindow(SDL_Window* window, GL_Context &context)
: tnf(-1.0)
, last_frame(-1.0)
, rm(context)
, accum(0)
, w(0), h(0)
, window(window)
{
	int n = 0;
	SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE, &accum);
	SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &n); if (n < accum) accum = n;
	SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE, &n); if (n < accum) accum = n;
	SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &n); if (n < accum) accum = n;
}
PlotWindow::~PlotWindow(){ }

void PlotWindow::stop_animations(){ tnf = -1.0; }
void PlotWindow::start_animations(){ if (tnf <= 0.0) tnf = now() + 1.0/FPS; }

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
		for (int i : params) change_parameter(i, cnum(idx, -idy));
	}
	else
	{
		dx *= dt; dy *= dt; dz *= dt;
		auto type = plot.axis.type();

		SDL_Keymod m = SDL_GetModState();
		bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
		bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
		bool alt     = (m & (KMOD_LALT|KMOD_RALT));
		
		if (shift)
		{
			dz = absmax(dx, dy);
			dx = dy = 0.0;
		}

		if (type == Axis::Rect)
		{
			translate(dx, dy, dz, ctrl|alt ? Inrange : Axis);
		}
		else if (type != Axis::Invalid)
		{
			translate(dx, dy, dz, ctrl ? Axis : alt ? Inrange : Camera);
		}
	}

	int pani = 0;
	for (Parameter *p : plot.used_parameters())
	{
		if (!p->anim) continue;
		p->animate(t);
		++pani;
		plot.recalc(p);
	}

	draw();

	if (!pani && ikeys.empty() && plot.axis_type() != Axis::Invalid)
	{
		stop_animations();
		return;
	}

	double t1 = std::max(now(), t + spf);
	tnf += std::max(0.0, spf * std::ceil((t1-tnf)/spf-0.01));
}

bool PlotWindow::handle_event(const SDL_Event &e)
{
	switch (e.type)
	{
		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
				case SDL_WINDOWEVENT_CLOSE:
					if (e.window.windowID == SDL_GetWindowID(window))
					{
						// TODO: get SDL to ask before closing, so we can
						// confirm if file was changed!
						quit = true;
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
			auto type = plot.axis.type();
			if (type == Axis::Invalid) return false;
			auto buttons = e.motion.state;
			if (!(buttons & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK))) return true;
			bool b1 = buttons & SDL_BUTTON_LMASK;
			bool b2 = buttons & SDL_BUTTON_RMASK;
			bool b3 = buttons & SDL_BUTTON_MMASK;
			if (b1 && b2) { b3 = true; b1 = b2 = false; }
			
			double dx = e.motion.xrel, dy = e.motion.yrel, dz = 0.0;

			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
			bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
			bool alt     = (m & (KMOD_LALT|KMOD_RALT));
			
			if (shift)
			{
				dz = absmax(dx, dy);
				dx = dy = 0.0;
			}

			if (type == Axis::Rect)
				translate(dx, dy, dz, (b2||b3||alt||ctrl) ? Inrange : Axis);
			else if (b3)
				translate(dx, dy, dz, Inrange);
			else if (b2)
				translate(dx, dy, dz, (ctrl || alt) ? Inrange : Axis);
			else
				translate(dx, dy, dz, ctrl ? Axis : alt ? Inrange : Camera);
			return true;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			return true;
		case SDL_MOUSEWHEEL:
		{
			auto type = plot.axis.type();
			if (type == Axis::Invalid) return false;

			SDL_Keymod m = SDL_GetModState();
			bool shift   = (m & (KMOD_LSHIFT|KMOD_RSHIFT));
			bool ctrl    = (m & (KMOD_LCTRL|KMOD_RCTRL));
			bool alt     = (m & (KMOD_LALT|KMOD_RALT));

			if (e.wheel.preciseX != e.wheel.x || e.wheel.preciseY != e.wheel.y)
			{
				double dx = -5.0*e.wheel.preciseX, dy = 5.0*e.wheel.preciseY, dz = 0.0;

				int mx = -1, my = -1; SDL_GetMouseState(&mx, &my);

				if (shift)
				{
					dz = absmax(dx, dy);
					dx = dy = 0.0;
				}

				if (type == Axis::Rect)
				{
					translate(dx, dy, dz, ctrl|alt ? Inrange : Axis, mx, my);
				}
				else if (type != Axis::Invalid)
				{
					translate(dx, dy, dz, ctrl ? Axis : alt ? Inrange : Camera);
				}
			}
			else
			{
				int mx = -1, my = -1;
				auto buttons = SDL_GetMouseState(&mx, &my);
				bool b1 = buttons & SDL_BUTTON_LMASK;
				bool b2 = buttons & SDL_BUTTON_RMASK;

				double dx = -5.0*e.wheel.x, dy = -5.0*e.wheel.y;

				if (type == Axis::Rect)
				{
					translate(0, 0, dy, (b1 || b2 || ctrl || alt) ? Inrange : Axis, mx, my);
					translate(0, 0, dx, (b1 || b2 || ctrl || alt) ? Axis : Inrange, mx, my);
				}
				else if (b2 || ctrl)
				{
					translate(0, 0, dy, Axis);
					translate(0, 0, dx, Inrange);
				}
				else if (b1 || alt)
				{
					translate(0, 0, dy, Inrange);
					translate(0, 0, dx, Axis);
				}
				else
				{
					translate(0, 0, dy, Camera);
					translate(0, 0, dx, Axis);
				}
			}
			return true;
		}
		case SDL_MULTIGESTURE:
		{
			// UNTESTED: my trackpad does not send these...
			auto &g = e.mgesture;
			printf("MULT %g %g - %g %g\n", g.x, g.y, g.dDist, g.dTheta);
			//move(0, 0, -5.0*e.mgesture.dDist, 0);
			return true;
		}
	}
	return false;
}

bool PlotWindow::handle_key(SDL_Keysym keysym, bool release)
{
	auto key = keysym.sym;
	auto m   = keysym.mod;
	constexpr int SHIFT = 1, CTRL = 2, ALT = 4;
	const int  shift = (m & (KMOD_LSHIFT|KMOD_RSHIFT)) ? SHIFT : 0;
	const int  ctrl  = (m & (KMOD_LCTRL|KMOD_RCTRL)) ? CTRL : 0;
	const int  alt   = (m & (KMOD_LALT|KMOD_RALT)) ? ALT : 0;
	const int  mods  = shift + ctrl + alt;

	auto view = [this](double x, double y)
	{
		switch (plot.axis_type())
		{
			case Axis::Invalid:
			case Axis::Rect: return false;
			default: break;
		}
		undoForCam();
		plot.camera.set_angles(x, y, 0.0);
		redraw();
		return true;
	};

	if (!release) switch (key)
	{
		case SDLK_LEFT: case SDLK_RIGHT:
		case SDLK_UP:   case SDLK_DOWN:
		case SDLK_PLUS: case SDLK_MINUS:
			if (!ikeys.count(key)) ikeys[key] = 0.0;
			start_animations();
			return true;
	
		case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
		case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
			if (!keys.count(key)) redraw();
			keys.insert(key);
			return true;

		case SDLK_SPACE:
		{
			if (keys.empty()) return true;
			std::set<Parameter*> aps(plot.used_parameters());
			if (aps.empty()) return true;
			std::vector<Parameter*> ps(aps.begin(), aps.end());
			std::sort(ps.begin(), ps.end(), [&](Parameter *a, Parameter *b)->bool{ return a->name() < b->name(); });
			for (auto k : keys)
			{
				int i = (k == SDLK_0 ? 9 : k-SDLK_1);
				if (i < 0 || i > 9) continue;
				if (i >= ps.size()) continue;
				Parameter *p = ps[i];
				if (p->anim) p->anim_stop();
				else if (p->anim_start())
				{
					undoForParam(p);
					start_animations();
				}
			}
			return true;
		}

		case SDLK_PERIOD:
			for (Parameter *p : rns.all_parameters(true))
			{
				if (!p->anim) continue;
				p->anim_stop();
				plot.recalc(p);
			}
			return true;

		case SDLK_a: return !mods && toggleAxis();
		case SDLK_g: return !mods && toggleGrid();
		case SDLK_v: return cycleVFMode(shift ? -1 : 1);
		case SDLK_d: return !mods && toggleDisco();
		case SDLK_c:
			if (!mods) return toggleClip();
			if (mods == shift) return toggleClipCustom();
			break;
		case SDLK_l:
			if (!mods) return toggleClipLock();
			if (mods == shift) return resetClipLock();
			break;

		case SDLK_t: view(0.0, shift ? -90.0 : 90.0); return true;
		case SDLK_f: view(shift ? 180.0 : 0.0,  0.3); return true;
		case SDLK_s: view(shift ? 90.0 : -90.0, 0.3); return true;

		case SDLK_z:
			if (mods) break;
			undoForAxis();
			plot.axis.reset_center();
			recalc(plot);
			return true;
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
		start_animations();
		return;
	}
	

	bool dynamic = Preferences::dynamic();
	bool anim = dynamic && (!ikeys.empty() || SDL_GetMouseState(NULL,NULL) & (SDL_BUTTON_LMASK|SDL_BUTTON_RMASK|SDL_BUTTON_MMASK));
	if (!anim && !plot.at_full_quality()) plot.update(CH_UNKNOWN);
	GL_CHECK;

	int nt = Preferences::threads();
	if (nt < 1 || nt > 256) nt = n_cores;
	plot.draw(rm, nt, accum, anim);
	GL_CHECK;

	status();
	GL_CHECK;

	need_redraw = !plot.at_full_quality();
}

void PlotWindow::translate(double dx, double dy, double dz, PlotWindow::Zoom what, int mx, int my)
{
	if (fabs(dx) < 1e-8 && fabs(dy) < 1e-8 && fabs(dz) < 1e-8) return;
	
	auto type = plot.axis.type();
	if (type == Axis::Invalid) return;

	redraw();

	double f = exp(-dz * 0.02);
	double pixel = plot.pixel_size();
	
	if (type == Axis::Rect)
	{
		if (what == Inrange)
		{
			undoForInRange();
			dx *= pixel;
			dy *= pixel;
			plot.axis.in_move(dx, -dy);
			plot.axis.in_zoom(1.0/f);
			plot.update(CH_IN_RANGE);
		}
		else // Axis or Camera
		{
			auto &axis = plot.axis;
			undoForAxis();
			dx *= pixel;
			dy *= pixel;
			axis.move(-dx, dy, 0.0);
 
			// zoom with dz, but keep (mx,my) where it is
			double x0 = 2.0 * mx / (w-1) - 1.0; // [-1,1]
			double y0 = 2.0 * (h-1-my) / (h-1) - 1.0;
			double x1 = (x0 * axis.range(0)) + axis.center(0);
			double y1 = (y0 * axis.range(1)) + axis.center(1);
			axis.zoom(f);
			if (w > 1 && h > 1 && mx >= 0 && my >= 0 && mx < w && my < h)
			{
				double x2 = (x0 * axis.range(0)) + axis.center(0);
				double y2 = (y0 * axis.range(1)) + axis.center(1);
				axis.move(x1-x2, y1-y2, 0.0);
			}

			plot.update(CH_AXIS_RANGE);
		}
	}
	else switch (what)
	{
		case Inrange:
			undoForInRange();
			plot.axis.in_move(dx*0.01, -dy*0.01);
			plot.axis.in_zoom(1.0/f);
			plot.update(CH_IN_RANGE);
			break;

		case Camera:
			undoForCam();
			plot.camera.rotate(0.01 * dy, 0, 0.01 * dx);
			plot.camera.zoom(f);
			break;

		case Axis:
		{
			undoForAxis();
			float f2; plot.camera.scalefactor(0, f2);
			dx /= f2; dy /= f2;
			plot.camera.move(plot.axis, -dx*pixel, dy*pixel, 0);
			plot.axis.zoom(f);
			plot.update(CH_AXIS_RANGE);
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
	if (p->anim) return;

	undoForParam(p);

	static double last_param_change = -1.0; // increment integers every 0.25 seconds only
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
	current_status_height = 0.0f;
	std::set<Parameter*> aps(plot.used_parameters());
	if (aps.empty()) return;
	std::vector<Parameter*> ps(aps.begin(), aps.end());
	std::sort(ps.begin(), ps.end(), [&](Parameter *a, Parameter *b)->bool{ return a->name() < b->name(); });

	bool dark = plot.axis.options.background_color.lightness() < 0.55;
	double fs = 17.0f;
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
	#ifdef DRAW_STATUS_AT_TOP
	glVertex2d(0, 0);
	glVertex2d(w, 0);
	glVertex2d(w, rows*lh+vspace*(rows+1));
	glVertex2d(0, rows*lh+vspace*(rows+1));
	#else
	const double y0 = h-rows*lh-vspace*(rows+1);
	glVertex2d(0, y0);
	glVertex2d(w, y0);
	glVertex2d(w, h);
	glVertex2d(0, h);
	#endif
	glEnd();

	current_status_height = (float)(rows*lh+vspace*(rows+1));

	x = hspace; double y = vspace;
	for (GL_String *s : labels)
	{
		if (x > hspace && x + s->w() + hspace > w){ x = hspace; y += lh + vspace; }
		#ifdef DRAW_STATUS_AT_TOP
		s->draw2d(x, y, s->w(), s->h());
		#else
		s->draw2d(x, y0+y, s->w(), s->h());
		#endif
		x += s->w() + 2.0*hspace;
	}
	glMatrixMode(GL_PROJECTION); glPopMatrix();
	glMatrixMode(GL_MODELVIEW);  glPopMatrix();
	labelCache.finish();
}

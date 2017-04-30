#include "GL_Axis.h"
#include "../OpenGL/GL_String.h"
#include "../OpenGL/GL_Font.h"
#include "../OpenGL/GL_Util.h"
#include "AxisLabels.h"
#include "../Plot.h"

//----------------------------------------------------------------------------------------------------------------------
// drawing utils
//----------------------------------------------------------------------------------------------------------------------

static inline void drawLine(double x1, double y1, double z1, double x2, double y2, double z2, const Axis &axis)
{
	P3d p1(x1,y1,z1), p2(x2,y2,z2);
	P3f q1,q2;
	axis.map(p1, q1);
	axis.map(p2, q2);
	glVertex3fv(q1);
	glVertex3fv(q2);
}
#define line(x1, y1, x2, y2)  drawLine(x1, y1, 0.0, x2, y2, 0.0, axis)

static inline void circle(double cx, double cy, double r_, double /*pixel*/, const Axis &axis)
{
	P3f q;
	axis.map(P3d(cx, cy, 0.0), q);
	float r = (float)(r_ / axis.range(0));
	glBegin(GL_LINE_LOOP);
	const float f = 2.0f*(float)M_PI/200.0f;
	for (int i = 0; i < 200; ++i)
	{
		float a = i*f;
		glVertex3f(q.x + r*cosf(a), q.y + r*sinf(a), q.z);
	}
	glEnd();
}

//----------------------------------------------------------------------------------------------------------------------
// GL_Axis
//----------------------------------------------------------------------------------------------------------------------

void GL_Axis::draw(const Plot &plot) const
{
	bool smooth = glIsEnabled(GL_LINE_SMOOTH);
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);

	switch (axis.type())
	{
		case Axis::Invalid: return;
		case Axis::Rect:   glDepthMask(GL_FALSE); draw2D(); break;
		case Axis::Box:    glDepthMask(GL_TRUE);  draw3D(); break;
		case Axis::Sphere: glDepthMask(GL_TRUE);  drawSphere(); break;
	}
	
	// draw preimage range
	bool used = false;
	for (int i = 0, n = plot.number_of_graphs(); i < n; ++i)
	{
		Graph *g = plot.graph(i); if (!g || g->options.hidden) continue;
		if ((g->type() == R2_R2 || g->type() == C_C) && g->mode() == GM_Image)
		{
			used = true;
			break;
		}
		else if (g->type() == C_C && g->mode() == GM_Riemann)
		{
			used = true;
			break;
		}
	}
	
	axis.options.axis_color.set();
	if (used) switch (axis.type())
	{
		case Axis::Invalid: break;
			
		case Axis::Rect:
			glDisable(GL_LINE_SMOOTH);
			// fallthrough
			
		case Axis::Box:
		{
			P3d A(axis.in_min(0), axis.in_min(1), 0.0);
			P3d B(axis.in_min(0), axis.in_max(1), 0.0);
			P3d C(axis.in_max(0), axis.in_max(1), 0.0);
			P3d D(axis.in_max(0), axis.in_min(1), 0.0);
			P3f p;
			glBegin(GL_LINE_LOOP);
			axis.map(A, p); glVertex3fv(p);
			axis.map(B, p); glVertex3fv(p);
			axis.map(C, p); glVertex3fv(p);
			axis.map(D, p); glVertex3fv(p);
			glEnd();
			break;
		}
			
		case Axis::Sphere:
		{
			cnum A(axis.in_min(0), axis.in_min(1));
			cnum B(axis.in_min(0), axis.in_max(1));
			cnum C(axis.in_max(0), axis.in_max(1));
			cnum D(axis.in_max(0), axis.in_min(1));
			P3d p;
			const int N = 60;
			glBegin(GL_LINE_LOOP);
			for (int i = 0; i < N; ++i)
			{
				double t = (double)i / N; // not N-1
				riemann((1.0-t)*A + t*B, p);
				glVertex3dv(p);
			}
			for (int i = 0; i < N; ++i)
			{
				double t = (double)i / N;
				riemann((1.0-t)*B + t*C, p);
				glVertex3dv(p);
			}
			for (int i = 0; i < N; ++i)
			{
				double t = (double)i / N;
				riemann((1.0-t)*C + t*D, p);
				glVertex3dv(p);
			}
			for (int i = 0; i < N; ++i)
			{
				double t = (double)i / N;
				riemann((1.0-t)*D + t*A, p);
				glVertex3dv(p);
			}
			glEnd();
		}
	}
	
	if (smooth) glEnable(GL_LINE_SMOOTH);
}

//--- 2D axis ----------------------------------------------------------------------------------------------------------

void GL_Axis::draw2D() const
{
	// 2d maps use (x,y,0)
	
	// we always have an OpenGL coordinate system where x is in [-1,1] (to avoid precision problems)
	// our actual coordinates are described by the axis (and to draw, this must be mapped to [-1,1])
	
	double pixel = camera.pixel_size(axis);
	double x0 = axis.min(0), x1 = axis.max(0);
	double y0 = axis.min(1), y1 = axis.max(1);

	// origin for the axes is 0 as long as it's visible
	// otherwise we take the closest edge (which may or may not be on a tick)
	double border = 30.0*pixel; // minimum distance from axes to screen edges
	double ox = 0.0, oy = 0.0;
	double arrh = 20.0*pixel;
	if (ox < x0+border){ ox = x0+border; }
	if (ox > x1-border-arrh){ ox = x1-border-arrh; }
	if (oy < y0+border){ oy = y0+border; }
	if (oy > y1-border-arrh){ oy = y1-border-arrh; }

	// setup drawing
	const GL_Color &caxis = axis.options.axis_color;
	GL_Color cmajor = caxis * 0.25f; // colors for major
	GL_Color cminor = caxis * 0.1f; // and minor ticks
	glLineWidth(1.0);

	AxisLabels al(pixel, x1-x0 - 2.0*border - arrh);

	// draw major and minor grid lines
	if (al.valid())
	{
		if (axis.options.axis_grid == AxisOptions::AG_Cartesian)
		{
			glDisable(GL_LINE_SMOOTH);
			glBegin(GL_LINES);
			for (GridIterator g(al, x0, x1); !g.done(); ++g){ (g.is_minor() ? cminor : cmajor).set(); line(g.x(), y0, g.x(), y1); }
			for (GridIterator g(al, y0, y1); !g.done(); ++g){ (g.is_minor() ? cminor : cmajor).set(); line(x0, g.x(), x1, g.x()); }
			glEnd();
		}
		else if (axis.options.axis_grid == AxisOptions::AG_Polar)
		{
			double r = hypot(std::min(x1, std::max(x0, 0.0)), std::min(y1, std::max(y0, 0.0)));
			double r_min = r, r_max = r;
			r = hypot(x0, y0); if (r < r_min) r_min = r; if (r > r_max) r_max = r;
			r = hypot(x1, y0); if (r < r_min) r_min = r; if (r > r_max) r_max = r;
			r = hypot(x0, y1); if (r < r_min) r_min = r; if (r > r_max) r_max = r;
			r = hypot(x1, y1); if (r < r_min) r_min = r; if (r > r_max) r_max = r;
			
			for (GridIterator g(al, r_min, r_max); !g.done(); ++g)
			{
				(g.is_minor() ? cminor : cmajor).set();
				circle(0.0, 0.0, g.x(), pixel, axis);
			}
		}
	}
	
	// draw axes
	glDisable(GL_LINE_SMOOTH);
	caxis.set();
	draw_arrow(P2d(x0+0.5*border, oy), P2d(x1-x0-border, 0.0), arrh, axis);
	draw_arrow(P2d(ox, y0+0.5*border), P2d(0.0, y1-y0-border), arrh, axis);

	if (al.valid())
	{
		// draw axis ticks (before changing state for the labels!)
		glBegin(GL_LINES);
		for (LabelIterator l(al, x0+20.0*pixel, x1-border-arrh, 6.0*pixel); !l.done(); ++l)
		{
			double x = l.x();
			if (fabs(x-ox) < 20.0*pixel){ continue; }
			line(x, oy-5.0*pixel, x, oy+5.0*pixel);
			if (l.skipped()) continue;
		}
		for (LabelIterator l(al, y0+20.0*pixel, y1-border-arrh, -1.0); !l.done(); ++l)
		{
			double y = l.x();
			if (fabs(y-oy) < 20.0*pixel){ continue; }
			line(ox-5.0*pixel, y, ox+5.0*pixel, y);
		}
		glEnd();
		
		// draw labels
		
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		GL_Font font(axis.options.label_font);
		font.color = caxis;
		labelCache.font(font);
		labelCache.start();

		for (LabelIterator l(al, x0+20.0*pixel, x1-border-arrh, 6.0*pixel); !l.done(); ++l)
		{
			double x = l.x();
			if (fabs(x-ox) < 20.0*pixel){ continue; }
			if (l.skipped()) continue; // skip overlapping x labels
			labelCache.get(l.label())->draw(P3d(x, oy-8.0*pixel, 0.0), HCENTER, TOP, axis, pixel);
		}
		for (LabelIterator l(al, y0+20.0*pixel, y1-border-arrh, -1.0); !l.done(); ++l)
		{
			double y = l.x();
			if (fabs(y-oy) < 20.0*pixel){ continue; }
			labelCache.get(l.label())->draw(P3d(ox-8.0*pixel, y, 0.0), RIGHT, VCENTER, axis, pixel);
		}

		labelCache.finish();
		glDisable(GL_TEXTURE_RECTANGLE_EXT); // enabled by GL_String
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//--- 3D axis ----------------------------------------------------------------------------------------------------------

void GL_Axis::draw3D() const
{
	double pixel = camera.pixel_size(axis);
	double border = 50.0*pixel; // minimum distance from text to axis ends

	// origin for the axes is always center so rotation does not look too weird
	double x0 = axis.min(0), x1 = axis.max(0), cx = axis.center(0);
	double y0 = axis.min(1), y1 = axis.max(1), cy = axis.center(1);
	double z0 = axis.min(2), z1 = axis.max(2), cz = axis.center(2);
	
	// box
	glLineWidth(1.0f);
	axis.options.axis_color.set();

	glBegin(GL_LINES);
	drawLine(x0, y0, z0, x1, y0, z0, axis);
	drawLine(x0, y1, z0, x1, y1, z0, axis);
	drawLine(x0, y0, z1, x1, y0, z1, axis);
	drawLine(x0, y1, z1, x1, y1, z1, axis);
	
	drawLine(x0, y0, z0, x0, y1, z0, axis);
	drawLine(x1, y0, z0, x1, y1, z0, axis);
	drawLine(x0, y0, z1, x0, y1, z1, axis);
	drawLine(x1, y0, z1, x1, y1, z1, axis);
	
	drawLine(x0, y0, z0, x0, y0, z1, axis);
	drawLine(x1, y0, z0, x1, y0, z1, axis);
	drawLine(x0, y1, z0, x0, y1, z1, axis);
	drawLine(x1, y1, z0, x1, y1, z1, axis);
	glEnd();

	// XYZ-labels
	labelCache.start();
	GL_Font font(axis.options.label_font);
	font.size *= 2.0f;
	font.color = axis.options.axis_color;
	labelCache.font(font);
	
	labelCache.get("X")->draw(P3d(cx, y0, z0 - 8.0*pixel), P3d(1.0,  0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER,    TOP, axis, pixel);
	labelCache.get("X")->draw(P3d(cx, y1, z0 - 8.0*pixel), P3d(-1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER,    TOP, axis, pixel);
	labelCache.get("X")->draw(P3d(cx, y0, z1 + 8.0*pixel), P3d(1.0,  0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
	labelCache.get("X")->draw(P3d(cx, y1, z1 + 8.0*pixel), P3d(-1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
	labelCache.get("Y")->draw(P3d(x0, cy, z0 - 8.0*pixel), P3d(0.0, -1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER,    TOP, axis, pixel);
	labelCache.get("Y")->draw(P3d(x1, cy, z0 - 8.0*pixel), P3d( 0.0, 1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER,    TOP, axis, pixel);
	labelCache.get("Y")->draw(P3d(x0, cy, z1 + 8.0*pixel), P3d(0.0, -1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
	labelCache.get("Y")->draw(P3d(x1, cy, z1 + 8.0*pixel), P3d( 0.0, 1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
	labelCache.get("Z")->draw(P3d(x0 - 8.0*pixel, y0, cz), P3d(1.0,  0.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT,  VCENTER, axis, pixel);
	labelCache.get("Z")->draw(P3d(x1 + 8.0*pixel, y1, cz), P3d(-1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT,  VCENTER, axis, pixel);
	labelCache.get("Z")->draw(P3d(x1, y0 - 8.0*pixel, cz), P3d(0.0,  1.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT,  VCENTER, axis, pixel);
	labelCache.get("Z")->draw(P3d(x0, y1 + 8.0*pixel, cz), P3d(0.0, -1.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT,  VCENTER, axis, pixel);

	AxisLabels al(pixel, x1-x0 - 2.0*border);

	/*// grid
	(axis.options.axis_color * 0.2f).set();
	if (al.valid())
	{
		for (GridIterator gx(al, x0, x1); !gx.done(); ++gx)
		{
			if (gx.is_minor()) continue;
			for (GridIterator gy(al, y0, y1); !gy.done(); ++gy)
			{
				if (gy.is_minor()) continue;
				drawLine(gx.x(), gy.x(), z0, gx.x(), gy.x(), z1, axis);
			}
		}
		for (GridIterator gx(al, x0, x1); !gx.done(); ++gx)
		{
			if (gx.is_minor()) continue;
			for (GridIterator gz(al, z0, z1); !gz.done(); ++gz)
			{
				if (gz.is_minor()) continue;
				drawLine(gx.x(), y0, gz.x(), gx.x(), y1, gz.x(), axis);
			}
		}
		for (GridIterator gy(al, y0, y1); !gy.done(); ++gy)
		{
			if (gy.is_minor()) continue;
			for (GridIterator gz(al, z0, z1); !gz.done(); ++gz)
			{
				if (gz.is_minor()) continue;
				drawLine(x0, gy.x(), gz.x(), x1, gy.x(), gz.x(), axis);
			}
		}
	}
	axis.options.axis_color.set(); */
	
	// tick marks and value labels
	if (al.valid())
	{
		for (LabelIterator l(al, x0+border, x1-border, 9.0*pixel); !l.done(); ++l)
		{
			double x = l.x();
			if (fabs(x-cx) < 20.0*pixel*(l.max_len()/2)) continue;
			glBegin(GL_LINES);
			drawLine(x, y0, z0-5*pixel, x, y0, z0+5*pixel, axis);
			drawLine(x, y1, z0-5*pixel, x, y1, z0+5*pixel, axis);
			drawLine(x, y0, z1-5*pixel, x, y0, z1+5*pixel, axis);
			drawLine(x, y1, z1-5*pixel, x, y1, z1+5*pixel, axis);
			glEnd();
			if (l.skipped()) continue;
			GL_String *label = labelCache.get(l.label());
			label->draw(P3d(x, y0, z0-8.0*pixel), P3d( 1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, TOP,    axis, pixel);
			label->draw(P3d(x, y1, z0-8.0*pixel), P3d(-1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, TOP,    axis, pixel);
			label->draw(P3d(x, y0, z1+8.0*pixel), P3d( 1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
			label->draw(P3d(x, y1, z1+8.0*pixel), P3d(-1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
		}
		for (LabelIterator l(al, y0+border, y1-border, 9.0*pixel); !l.done(); ++l)
		{
			double y = l.x();
			if (fabs(y-cy) < 20.0*pixel*(l.max_len()/2)) continue;
			glBegin(GL_LINES);
			drawLine(x0, y, z0-5*pixel, x0, y, z0+5*pixel, axis);
			drawLine(x1, y, z0-5*pixel, x1, y, z0+5*pixel, axis);
			drawLine(x0, y, z1-5*pixel, x0, y, z1+5*pixel, axis);
			drawLine(x1, y, z1-5*pixel, x1, y, z1+5*pixel, axis);
			glEnd();
			if (l.skipped()) continue;
			GL_String *label = labelCache.get(l.label());
			label->draw(P3d(x0, y, z0-8.0*pixel), P3d(0.0, -1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, TOP,    axis, pixel);
			label->draw(P3d(x1, y, z0-8.0*pixel), P3d(0.0,  1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, TOP,    axis, pixel);
			label->draw(P3d(x0, y, z1+8.0*pixel), P3d(0.0, -1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
			label->draw(P3d(x1, y, z1+8.0*pixel), P3d(0.0,  1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
		}
		for (LabelIterator l(al, z0+border, z1-border, -1); !l.done(); ++l)
		{
			double z = l.x();
			if (fabs(z-cz) < 20.0*pixel*1.5) continue;
			glBegin(GL_LINES);
			drawLine(x0-5*pixel, y0, z, x0+5*pixel, y0, z, axis);
			drawLine(x1, y0-5*pixel, z, x1, y0+5*pixel, z, axis);
			drawLine(x1-5*pixel, y1, z, x1+5*pixel, y1, z, axis);
			drawLine(x0, y1-5*pixel, z, x0, y1+5*pixel, z, axis);
			glEnd();
			if (l.skipped()) continue;
			GL_String *label = labelCache.get(l.label());
			label->draw(P3d(x0-8.0*pixel, y0, z), P3d( 1.0,  0.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT, VCENTER, axis, pixel);
			label->draw(P3d(x1, y0-8.0*pixel, z), P3d( 0.0,  1.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT, VCENTER, axis, pixel);
			label->draw(P3d(x1+8.0*pixel, y1, z), P3d(-1.0,  0.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT, VCENTER, axis, pixel);
			label->draw(P3d(x0, y1+8.0*pixel, z), P3d( 0.0, -1.0, 0.0), P3d(0.0, 0.0, 1.0), RIGHT, VCENTER, axis, pixel);
		}
	}
	labelCache.finish();
}

//--- 3D parametric axis -----------------------------------------------------------------------------------------------

/*void GL_Axis::draw3D_cross() const
{
	double pixel = camera.pixel_size(axis);
	double x0 = axis.min(0), x1 = axis.max(0), cx = axis.center[0];
	double y0 = axis.min(1), y1 = axis.max(1), cy = axis.center[1];
	double z0 = axis.min(2), z1 = axis.max(2), cz = axis.center[2];
	
	// origin for the axes is always center so rotation does not look too weird
	double border = 50.0*pixel; // minimum distance from text to axis ends
	double ox = cx, oy = cy, oz = cz;
	
	// setup drawing
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	
	// draw axes and arrows
	glLineWidth(1.5);
	const GL_Color &axisColor = axis.options.axis;
	axisColor.set();
	
	float arl = float(25.0 * pixel); // length of the arrow's tip
	P3d p1, p2;
	p1.set(x0, oy, oz);
	p2.set(x1-x0, 0.0f, 0.0f);
	draw_arrow(p1, p2, arl, axis);
	
	p1.set(ox, y0, oz);
	p2.set(0.0f, y1-y0, 0.0f);
	draw_arrow(p1, p2, arl, axis);
	
	p1.set(ox, oy, z0);
	p2.set(0.0f, 0.0f, z1-z0);
	draw_arrow(p1, p2, arl, axis);
	
	glDisable(GL_POLYGON_SMOOTH);
	
	
	int    subdiv;
	double major = find_ticks(pixel, subdiv);
	double labels = major;
	double xr = x1 - x0 - 2.0*border;
	if (xr < 6.0 * labels){ labels = major /  2.0; }
	if (xr < 6.0 * labels){ labels = major /  5.0; }
	if (xr < 6.0 * labels){ labels = major / 10.0; }
	
	labelCache.start();
	GL_Font font("Lucida Grande", 18.0f);
	font.color = axisColor;
	labelCache.font(font);
	
	// XYZ-labels
	labelCache.get("X")->draw(P3d(x1 + 5.0*pixel, oy, oz), P3d(1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), LEFT,   VCENTER, axis, pixel);
	labelCache.get("Y")->draw(P3d(ox, y1 + 5.0*pixel, oz), P3d(0.0, 1.0, 0.0), P3d(0.0, 0.0, 1.0), LEFT,   VCENTER, axis, pixel);
	labelCache.get("Z")->draw(P3d(ox, oy, z1 + 5.0*pixel), P3d(1.0, 0.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, BOTTOM, axis, pixel);
	
	// tick marks and value labels
	glLineWidth(1.5);
	for (double x = first_tick(x0+border, labels); x+border <= x1; x += labels)
	{
		if (fabs(x-ox) < 20.0*pixel) continue;
		drawLine(x, oy, oz-5*pixel, x, oy, oz+5*pixel, axis);
		labelCache.get(format("%g", x))->draw(P3d(x, oy, oz-8.0*pixel), HCENTER, TOP, axis, pixel);
	}
	for (double y = first_tick(y0+border, labels); y+border <= y1; y += labels)
	{
		if (fabs(y-oy) < 20.0*pixel) continue;
		drawLine(ox, y, oz-5*pixel, ox, y, oz+5*pixel, axis);
		labelCache.get(format("%g", y))->draw(P3d(ox, y, oz-8.0*pixel), P3d(0.0, 1.0, 0.0), P3d(0.0, 0.0, 1.0), HCENTER, TOP, axis, pixel);
	}
	for (double z = first_tick(z0+border, labels); z+border <= z1; z += labels)
	{
		if (fabs(z-oz) < 20.0*pixel) continue;
		drawLine(ox-5*pixel, oy, z, ox+5*pixel, oy, z, axis);
		labelCache.get(format("%g", z))->draw(P3d(ox-8.0*pixel, oy, z), RIGHT, VCENTER, axis, pixel);
	}
	
	labelCache.finish();
}
*/

//--- Riemann axis -----------------------------------------------------------------------------------------------------

#define CDIV 30 /* subdivisions/2 for drawing circles*/

void GL_Axis::drawSphere() const
{
	double pixel = 0.0025;//camera.pixel_size(axis);
	
	glLineWidth(1.0);
	axis.options.axis_color.set();
	
	const GL_Color &caxis = axis.options.axis_color;
	GL_Color cmajor = caxis * 0.3f; // colors for major
	GL_Color cminor = caxis * 0.2f;  // and minor lines

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	double r = 1.001;
	glScaled(r, r, r);

	int rdiv = 20, zdiv = 20;

	assert(zdiv % 2 == 0);
	for (int i = 1; i < zdiv; ++i) // constant |z| lines
	{
		double z = cos(M_PI * i / zdiv);
		double rz = sqrt(1.0 - z*z);

		glBegin(GL_LINE_LOOP);
		(i == zdiv/2 ? cmajor : cminor).set();
		for (int j = 0; j < 2*CDIV; ++j)
		{
			double p = 2.0 * M_PI * j / (2*CDIV);
			double sp, cp; sincos(p, sp, cp);
			glVertex3d(rz * cp, rz * sp, z);
		}
		glEnd();
	}

	double t1 = M_PI / zdiv;
	assert(rdiv % 4 == 0);
	for (int i = 0; i < rdiv; ++i) // constant arg(z) lines
	{
		double p = 2.0 * M_PI * i / rdiv;
		double sp, cp; sincos(p, sp, cp);
		
		glBegin(GL_LINE_STRIP);
		bool major = (4*i % rdiv == 0);
		(major ? cmajor : cminor).set();
		for (int j = 0; j <= CDIV; ++j)
		{
			double t = (M_PI - (major ? 0.0 : 2.0*t1)) * j / CDIV - 0.5*M_PI + (major ? 0.0 : t1);
			double st, ct; sincos(t, st, ct);
			glVertex3d(cp * ct, sp * ct, st);
		}
		glEnd();
	}
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// labels
	labelCache.start();
	GL_Font font(axis.options.label_font);
	font.size *= 2.0f;
	font.color = caxis;
	caxis.set();
	labelCache.font(font);
	
	labelCache.get("0")->draw_fixed(P3d(0.0, 0.0, 1.0), P3d( 1.0,  0.0, 0.0), P3d(0.0, 1.0, 0.0), HCENTER, VCENTER, pixel);
	labelCache.get("∞")->draw_fixed(P3d(0.0, 0.0,-1.0), P3d(-1.0,  0.0, 0.0), P3d(0.0, 1.0, 0.0), HCENTER, VCENTER, pixel);

	labelCache.get( "1")->draw_fixed(P3d( 1.05, 0.0, 0.0), P3d(1.0,  0.0, 0.0), P3d(0.0, 1.0, 0.0), LEFT,  VCENTER, pixel);
	labelCache.get("-1")->draw_fixed(P3d(-1.05, 0.0, 0.0), P3d(1.0,  0.0, 0.0), P3d(0.0, 1.0, 0.0), RIGHT, VCENTER, pixel);

	labelCache.get( "i")->draw_fixed(P3d(0.0, 1.05, 0.0), P3d(1.0,  0.0, 0.0), P3d(0.0, 1.0, 0.0), HCENTER, BOTTOM, pixel);
	labelCache.get("-i")->draw_fixed(P3d(0.0,-1.05, 0.0), P3d(1.0,  0.0, 0.0), P3d(0.0, 1.0, 0.0), HCENTER, TOP,    pixel);

	labelCache.finish();
}

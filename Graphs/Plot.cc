#include "Plot.h"
#include "OpenGL/GL_Color.h"
#include "OpenGL/GL_Font.h"
#include "../Utility/System.h"
#include "../Engine/Namespace/RootNamespace.h"
#include "OpenGL/GL_Context.h"
#include <GL/gl.h>
#include <cassert>
#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// PlotOptions
//----------------------------------------------------------------------------------------------------------------------

void PlotOptions::save(Serializer &s) const
{
	s.enum_(aa_mode, AA_Off, AA_8x);
	s.double_(fog);
	clip.save(s);
}
void PlotOptions::load(Deserializer &s)
{
	s.enum_(aa_mode, AA_Off, AA_8x);
	s.double_(fog);
	clip.load(s);
}

//----------------------------------------------------------------------------------------------------------------------
// Plot
//----------------------------------------------------------------------------------------------------------------------

Plot::Plot(const Plot &p)
: ns(*new RootNamespace)
#ifndef _WINDOWS
, PropertyList()
#endif
, options(p.options)
, axis(p.axis)
, camera(p.camera)
, gl_axis(axis, camera)
, current(p.current)
, anim_qf(p.anim_qf)
, last_update_was_full_quality(false)
, graphs(p.graphs.size(), NULL)
{
	try
	{
		assert(p.ns.isRoot());

		for (const Element *e : p.ns)
		{
			e->copy(ns);
		}

		for (size_t i = 0, n = graphs.size(); i < n; ++i)
		{
			assert(p.graphs[i] != NULL);
			graphs[i] = new Graph(*p.graphs[i], *this);
		}
	}
	catch (...)
	{
		for (Graph *g : graphs) delete g;
		delete &ns;
		throw;
	}
}

Plot::~Plot()
{
	for (Graph *g : graphs) delete g;
}

void Plot::save(Serializer &s) const
{
	axis.save(s);
	camera.save(s);
	options.save(s);
	s.uint64_(graphs.size());
	for (const Graph *g : graphs) g->save(s);
}
void Plot::load(Deserializer &s)
{
	for (Graph *g : graphs) delete g;
	graphs.clear();

	axis.load(s);
	camera.load(s);
	options.load(s);
	uint64_t n;
	s.uint64_(n);
	for (uint64_t i = 0; i < n; ++i)
	{
		graphs.push_back(new Graph(*this));
		graphs[(size_t)i]->load(s);
	}
	current = 0;
	update_axis();
}

//----------------------------------------------------------------------------------------------------------------------
// Managing graphs
//----------------------------------------------------------------------------------------------------------------------

Graph *Plot::add_graph(bool make_current)
{
	Graph *cg = current_graph();
	Graph *g = cg ? new Graph(*cg) : new Graph(*this);
	graphs.push_back(g);
	if (!cg)
	{
		g->options.texture = IP_COLORS;
		g->options.reflection_texture = IP_XOR;
	}
	if (make_current) current = (int)graphs.size() - 1;
	return g;
}
void Plot::add_graph(Graph *g, bool make_current)
{
	assert(&g->plot == this);
	graphs.push_back(g);
	if (make_current) current = (int)graphs.size() - 1;
	g->update(CH_UNKNOWN);
}

void Plot::delete_graph(Graph *g)
{
	for (int i = 0, n = (int)graphs.size(); i < n; ++i)
	{
		if (graphs[i] == g)
		{
			if (i == current && i == n-1) --current;
			graphs.erase(graphs.begin() + i);
			delete g;
			return;
		}
	}
	assert(false);
	delete g;
}
void Plot::set_current_graph(Graph *g)
{
	for (int i = 0, n = (int)graphs.size(); i < n; ++i)
	{
		if (graphs[i] == g)
		{
			current = i;
			return;
		}
	}
	assert(false);
}
void Plot::set_current_graph(int i)
{
	if (i >= (int)graphs.size()) i = (int)graphs.size() - 1;
	if (i < 0) i = 0;
	current = i;
}

void Plot::update_axis()
{
	Axis::Type old = axis.type();
	axis.type(axis_type());
	if (axis.type() != old) update(CH_AXIS_TYPE);
	
	bool upright = false;
	for (const Graph *g : graphs)
	{
		if (g->options.hidden) continue;
		if (g->axisType() == Axis::Box && g->wantUprightView())
		{
			upright = true;
			break;
		}
	}
	camera.aligned(upright);
	camera.orthogonal(axis.type() == Axis::Rect || axis.type() == Axis::Invalid);
}

void Plot::draw(GL_RM &rm, int n_threads, bool accum_ok, bool for_animation) const
{
	double t0 = now();
	size_t updated = 0, visible = 0;
	
	if (anim_qf < 0.0) anim_qf = 0.0; else if (anim_qf > 0.999) anim_qf = 0.999;
	double q = (for_animation ? anim_qf : 1.0);
	
	std::vector<GL_Graph*> area_graphs, line_graphs, image_graphs, all_graphs;
	for (const Graph *g : graphs)
	{
		if (g->options.hidden) continue;
		GL_Graph *gl = g->gl_graph();
		if (!gl) continue;
		all_graphs.push_back(gl);
		(g->isColor() ? image_graphs :
		 g->isArea()  ? area_graphs  :
		 line_graphs).push_back(gl);
		
		++visible;
		
		if (g->needs_update())
		{
			try
			{
				gl->update(n_threads, q);
				++updated;
				g->clear_update();
			}
			catch(const std::bad_alloc &)
			{
				// ignore
				#ifdef DEBUG
				std::cerr << "allocation failed!" << std::endl;
				#endif
			}
		}
	}
	
	if (axis.type() == Axis::Box || axis.type() == Axis::Sphere)
	{
		std::sort(all_graphs.begin(), all_graphs.end(), [](GL_Graph *a, GL_Graph *b)
		{
			return a->opacity() < b->opacity();
		});
	}
	
	rm.start_drawing(translate(options.aa_mode, accum_ok));
	try {

		GL_CHECK;

		glDisable(GL_DITHER);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		if ((axis.type() == Axis::Box || axis.type() == Axis::Sphere) && options.fog > 1e-4)
		{
			glEnable(GL_FOG);
			glFogi(GL_FOG_MODE, GL_EXP2);
			glFogfv(GL_FOG_COLOR, axis.options.background_color.v);
			glFogf(GL_FOG_DENSITY, (float)options.fog);
		}
		else
		{
			glDisable(GL_FOG);
		}

		GL_CHECK;

		for (bool first = true; rm.draw_subframe(camera); first = false)
		{
			GL_CHECK;

			axis.options.background_color.set_clear();
			glClearDepth(1.0);
			glDepthMask(GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (options.aa_mode == AA_Lines)
			{
				glEnable(GL_LINE_SMOOTH);
				glEnable(GL_POINT_SMOOTH);
				glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
				glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
			}
			else
			{
				glDisable(GL_LINE_SMOOTH);
				glDisable(GL_POINT_SMOOTH);
			}

			switch (axis.type())
			{
				case Axis::Rect:
				{
					axis.options.light.setup(false);

					glDisable(GL_DEPTH_TEST);
					for (GL_Graph *gl : image_graphs) gl->draw(rm);

					glEnable(GL_DEPTH_TEST); // so area graphs can self-clip
					for (GL_Graph *gl : area_graphs)
					{
						glClear(GL_DEPTH_BUFFER_BIT);
						gl->draw(rm);
					}
					glDisable(GL_DEPTH_TEST);

					if (!axis.options.hidden) gl_axis.draw(*this);

					for (GL_Graph *gl : line_graphs) gl->draw(rm);

					break;
				}

				case Axis::Box:
				case Axis::Sphere:
				{
					glEnable(GL_DEPTH_TEST);
					axis.options.light.setup(true);

					bool depth_sort = false;
					if (first) settings.get("depthSorting", depth_sort);

					P3f view = camera.view_vector();
					if (!options.clip.locked()) options.clip.normal(view);
					options.clip.set(axis);

					for (GL_Graph *gl : all_graphs)
					{
						if (depth_sort) gl->depth_sort(view);
						gl->draw(rm);
					}

					options.clip.unset();

					if (!axis.options.hidden)
					{
						gl_axis.draw(*this);
						options.clip.draw(axis);
					}

					break;
				}

				case Axis::Invalid:
				{
					break;
				}
			}
		}

		GL_CHECK;
	}
	catch (...)
	{
		rm.end_drawing();
		throw;
	}
	rm.end_drawing();
	GL_CHECK;

	if (!visible) last_update_was_full_quality = true;
	
	if (updated > 0)
	{
		#ifdef DEBUG
		//std::cerr << "Plot update " << updated << "/" << graphs.size() << " @ ";
		#endif
		if (for_animation && !last_update_was_full_quality) // ignore the first one
		{
			#ifdef DEBUG
			//std::cerr << anim_qf << std::endl;
			#endif
			double dt = now() - t0, fps = 1.0/dt;
			if      (fps <  20.0) anim_qf -= 0.2;
			else if (fps <  40.0) anim_qf -= 0.05;
			else if (fps > 100.0) anim_qf += 0.2;
			else if (fps >  80.0) anim_qf += 0.05;
		}
		last_update_was_full_quality = !for_animation;
	}
}

std::set<Parameter*> Plot::used_parameters() const
{
	std::set<Parameter*> ps;
	for (const Graph *g : graphs)
	{
		std::set<Parameter*> gps = g->used_parameters();
		ps.insert(gps.begin(), gps.end());
	}
	return ps;
}

Axis::Type Plot::axis_type() const
{
	Axis::Type T = Axis::Invalid;
	for (const Graph *g : graphs)
	{
		if (g->options.hidden) continue;
		Axis::Type t = g->axisType();
		switch(t)
		{
			case Axis::Invalid: continue;
			case Axis::Rect:
				if (T == Axis::Invalid) T = t;
				if (T == Axis::Sphere) return Axis::Invalid;
				break;

			case Axis::Box:
				if (T == Axis::Sphere) return Axis::Invalid;
				T = t;
				break;
				
			case Axis::Sphere:
				if (T == Axis::Box || T == Axis::Rect) return Axis::Invalid;
				T = t;
				break;
		}
	}
	return T;
}

#pragma once
#include "Graph.h"
#include "Properties.h"
#include "Geometry/Axis.h"
#include "Geometry/Camera.h"
#include "../Persistence/Serializer.h"
#include "Graphics/GL_Axis.h"
#include "Graphics/GL_Graph.h"
#include "OpenGL/GL_ClippingPlane.h"
#include "OpenGL/GL_AAMode.h"
#include <vector>
#include <set>

struct PlotOptions : public Serializable
{
	PlotOptions() : aa_mode(AA_Lines), fog(0.0){ }
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	AntialiasMode    aa_mode;
	double           fog; // 0..1
	mutable GL_ClippingPlane clip; // normal must be updated while the plane is unlocked
};

struct Plot : public Serializable
#ifndef _WINDOWS
	, public PropertyList
#endif
{
	Plot(Namespace &ns) : ns(ns), gl_axis(axis, camera), current(0), anim_qf(0.75), last_update_was_full_quality(false){ }
	Plot(const Plot &p); // allocates an independent RootNamespace, which must be deleted by the caller!
	~Plot();
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);

	void clear()
	{
		for (Graph *g : graphs) delete g;
		graphs.clear();
		current = -1;
		update_axis();
	}
	
	double               pixel_size() const{ return camera.pixel_size(axis); }
	Axis::Type           axis_type() const;
	std::set<Parameter*> used_parameters() const;
	int used_inrange() const
	{
		int R = 0; 
		for (Graph *g : graphs)
		{
			if (g->options.hidden) continue;
			int r = g->inRangeDimension();
			if (r > R) R = r;
		}
		return R;
	}

	Namespace           &ns;
	Axis                 axis;
	Camera               camera;
	PlotOptions          options;
	
	Graph *add_graph(bool make_current = true);
	void   add_graph(Graph *g, bool make_current = true);
	void   delete_graph(Graph *g);
	
	int    number_of_graphs() const{ return (int)graphs.size(); }
	int    number_of_visible_graphs() const{ int n = 0; for (Graph *g : graphs) if (!g->options.hidden) ++n; return n; }
	Graph *graph(int i) const{ return (i >= 0 && i < (int)graphs.size()) ? graphs[i] : NULL; }
	Graph *current_graph() const{ return graph(current); }
	int    current_graph_index() const{ return current; }
	void   set_current_graph(Graph *g);
	void   set_current_graph(int    i);
	
	void draw(GL_RM &rm, int n_threads, bool accum_ok, bool for_animation) const;
	void update_axis(); // sync axis to current plot settings
	void update(ChangeType t){ for (Graph *g : graphs) g->update(t); }
	void recalc(){ update(CH_UNKNOWN); }
	bool recalc(Parameter *p)
	{
		bool change = false;
		for (Graph *g : graphs)
		{
			if (!g->uses_parameter(*p)) continue;
			g->update(CH_PARAM);
			if (!g->options.hidden) change = true;
		}
		return change;
	}
	bool reparse(const std::string &name)
	{
		if (name.empty()) return false;
		bool change = false;
		for (Graph *g : graphs)
		{
			if (!g->uses_object(name)) continue;
			g->update(CH_EXPRESSION);
			if (!g->options.hidden) change = true;
		}
		return change;
	}
	bool needs_update() const{ for (Graph *g : graphs) if (!g->options.hidden && g->needs_update()) return true; return false; }
	bool at_full_quality() const{ return last_update_was_full_quality; }

#ifndef _WINDOWS
protected:
	virtual void init_properties();
	virtual const Namespace &pns() const { return ns; }
#endif

private:
	std::vector<Graph*> graphs;
	int                 current;
	GL_Axis             gl_axis;
	mutable double      anim_qf; // quality scale factor during animation
	mutable bool        last_update_was_full_quality; // draw last called with for_animation==false?
};

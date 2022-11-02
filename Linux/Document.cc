#include "Document.h"
#include "../Persistence/Serializer.h"

void Document::saveAs(const std::string &p) const
{
	FILE *F = fopen(p.c_str(), "w");
	if (!F) throw std::runtime_error(std::string("can't open file for writing: ") + p);
	try
	{
		FileWriter fw(F);
		Serializer  w(fw);
		rns.save(w);
		plot.save(w);
		if (w.version() >= FILE_VERSION_1_8)
		{
			uint32_t box_state = 0; // from mac version
			w.uint32_(box_state);
		}
		w.marker_("EOF.");
	}
	catch(...)
	{
		fclose(F);
		throw;
	}
	fclose(F);
	path = p;
	ut.file_was_saved();
}

void Document::load(const std::string &p)
{
	FILE *F = fopen(p.c_str(), "r");
	if (!F) throw std::runtime_error(std::string("can't open file for reading: ") + p);
	try
	{
		FileReader   fr(F);
		Deserializer s(fr);
		plot.clear(); // TODO: don't modify on fail
		rns.clear();
		rns.load(s);
		plot.load(s);
		uint32_t box_state;
		if (s.version() >= FILE_VERSION_1_8) s.uint32_(box_state);
		s.marker_("EOF.");
		assert(s.done());
	}
	catch(...)
	{
		fclose(F);
		throw;
	}
	fclose(F);
	path = p;
	ut.file_was_loaded();
}

void Document::recalc(Plot &plot)
{
	plot.update(-1);
	redraw();
}
void Document::recalc(Graph *g)
{
	g->update(-1);
	redraw();
}

//------------------------------------------------------------------------------------------------
// Actions
//------------------------------------------------------------------------------------------------
static inline void toggle(bool &b) { b = !b; }

bool Document::toggleAxis()
{
	if (plot.axis_type() == Axis::Invalid) return false;
	toggle(plot.axis.options.hidden);
	redraw();
	ut.reg("Toggle Axis", [this]{ toggleAxis(); }, &plot.axis.options.hidden, TOGGLE_OP);
	return true;
}

bool Document::toggleDisco()
{
	Graph *g = plot.current_graph(); if (!g) return false;
	toggle(g->options.disco);
	recalc(g);
	ut.reg("Toggle Discontinuities", [this]{ toggleDisco(); }, &g->options.disco, TOGGLE_OP);
	return true;
}

bool Document::toggleClip()
{
	Graph *g = plot.current_graph(); if (!g) return false;
	g->clipping(!g->clipping());
	recalc(g);
	ut.reg("Toggle Clipping", [this]{ toggleClip(); }, &g->options.clip_graph_to_axis, TOGGLE_OP);
	return true;
}

bool Document::toggleClipCustom()
{
	plot.options.clip.on(!plot.options.clip.on());
	recalc(plot);
	ut.reg("Toggle Custom Clipping Plane", [this]{ toggleClipCustom(); }, &plot.options.clip, TOGGLE_OP);
	return true;
}
bool Document::toggleClipLock()
{
	bool v0 = plot.options.clip.locked();
	P3f  n0 = plot.options.clip.normal();
	ut.reg("Change Custom Clipping Plane", [this,n0,v0]{ setClipLock(v0,n0); }, &plot.options.clip, 1);
	plot.options.clip.locked(!v0);
	redraw();
	return true;
}
bool Document::setClipLock(bool lock, const P3f &normal)
{
	bool v0 = plot.options.clip.locked();
	P3f n0 = plot.options.clip.normal();
	ut.reg("Change Custom Clipping Plane", [this,n0,v0]{ setClipLock(v0,n0); }, &plot.options.clip, 1);
	plot.options.clip.locked(lock);
	plot.options.clip.normal(normal);
	recalc(plot);
	return true;
}
bool Document::resetClipLock()
{
	bool v0 = plot.options.clip.locked();
	P3f n0 = plot.options.clip.normal();
	ut.reg("Change Custom Clipping Plane", [this,n0,v0]{ setClipLock(v0,n0); }, &plot.options.clip, 1);
	plot.options.clip.locked(true);
	plot.options.clip.normal(plot.camera.view_vector());
	recalc(plot);
	return true;
}
bool Document::setClipDistance(float v)
{
	float v0 = plot.options.clip.distance();
	if (fabs(v-v0) < 1e-12) return true;
	plot.options.clip.distance(v);
	recalc(plot);
	ut.reg("Change Clip Distance", [this,v0]{ setClipDistance(v0); }, &plot.options.clip, 2);
	return true;
}

bool Document::setGrid(GridStyle v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.grid_style;
	if (v0 == v) return true;
	g->options.grid_style = v;
	recalc(g);
	ut.reg("Change Grid", [this,v0]{ setGrid(v0); }, &g->options.grid_style);
	return true;
}
bool Document::toggleGrid()
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.grid_style;
	g->options.grid_style = (g->options.grid_style == Grid_Off ? Grid_On : Grid_Off);
	recalc(g);
	if (v0 == Grid_Full)
		ut.reg("Change Grid", [this,v0]{ setGrid(v0); }, &g->options.grid_style);
	else
		ut.reg("Toggle Grid", [this]{ toggleGrid(); }, &g->options.grid_style, TOGGLE_OP);
	return true;
}

bool Document::setQuality(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.quality;
	if (fabs(v-v0) < 1e-12) return true;
	g->options.quality = v;
	recalc(g);
	ut.reg("Change Quality", [this,v0]{ setQuality(v0); }, &g->options.quality);
	return true;
}
bool Document::setGridDensity(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.grid_density;
	if (fabs(v-v0) < 1e-12) return true;
	g->options.grid_density = v;
	recalc(g);
	ut.reg("Change Grid Density", [this,v0]{ setGridDensity(v0); }, &g->options.grid_density);
	return true;
}
bool Document::setMeshDensity(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.mask.density();
	if (fabs(v-v0) < 1e-12) return true;
	g->options.mask.density(v);
	redraw();
	ut.reg("Change Mesh Density", [this,v0]{ setMeshDensity(v0); }, &g->options.mask);
	return true;
}
bool Document::setHistoScale(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.hist_scale;
	if (fabs(v-v0) < 1e-12) return true;
	g->options.hist_scale = v;
	recalc(g);
	ut.reg("Change Histogram Scale", [this,v0]{ setHistoScale(v0); }, &g->options.hist_scale);
	return true;
}

bool Document::setDisplayMode(ShadingMode m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto m0 = g->options.shading_mode;
	if (m == m0) return true;
	g->options.shading_mode = m;
	plot.update_axis();
	recalc(g);
	ut.reg("Change Display Mode", [this,m0]{ setDisplayMode(m0); });
	return true;
}
bool Document::setVFMode(VectorfieldMode m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto m0 = g->options.vf_mode;
	if (m == m0) return true;
	g->options.vf_mode = m;
	recalc(g);
	ut.reg("Change Vector Field Mode", [this,m0]{ setVFMode(m0); });
	return true;
}
bool Document::cycleVFMode(int d)
{
	assert(d == 1 || d == -1);
	Graph *g = plot.current_graph(); if (!g) return false;
	auto m0 = g->options.vf_mode;
	int m = d + (int)m0;
	if (m < 0) m = VF_LAST;
	if (m > VF_LAST) m = 0;

	g->options.vf_mode = (VectorfieldMode)m;
	recalc(g);
	ut.reg("Change Vector Field Mode", [this,m0]{ setVFMode(m0); }, &g->options.vf_mode);
	return true;
}

bool Document::setHistoMode(HistogramMode m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto m0 = g->options.hist_mode;
	if (m == m0) return true;
	g->options.hist_mode = m;
	plot.update_axis();
	recalc(g);
	ut.reg("Change Histogram Mode", [this,m0]{ setHistoMode(m0); });
	return true;
}

bool Document::setMeshMode(MaskStyle m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto m0 = g->options.mask.style();
	if (m == Mask_Custom) return false;
	if (m == m0) return true;
	if (m0 == Mask_Custom) return false; // todo: needs different undo
	g->options.mask.style(m);
	redraw();
	ut.reg("Change Mesh", [this,m0]{ setMeshMode(m0); });
	return true;
}

bool Document::setAxisGrid(AxisOptions::AxisGridMode m)
{
	auto m0 = plot.axis.options.axis_grid;
	if (m == m0) return true;
	plot.axis.options.axis_grid = m;
	redraw();
	ut.reg("Change Axis Grid", [this,m0]{ setAxisGrid(m0); });
	return true;
}

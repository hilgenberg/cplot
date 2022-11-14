#include "Document.h"
#include "../Persistence/Serializer.h"
#include "../Engine/Namespace/Parameter.h"
#include "../Engine/Namespace/UserFunction.h"

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

void Document::clear()
{
	plot.clear();
	rns.clear();
	path.clear();
	ut.file_was_loaded();
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

//-------------------------------------------------------------------------------

bool Document::setFog(double v)
{
	auto v0 = plot.options.fog;
	if (fabs(v-v0) < 1e-12) return true;
	plot.options.fog = v;
	redraw();
	ut.reg("Change Fog Strength", [this,v0]{ setFog(v0); }, &plot.options.fog);
	return true;
}

bool Document::setLineWidth(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = (g->usesLineColor() ? g->options.line_width : g->options.gridline_width);
	if (fabs(v-v0) < 1e-12) return true;
	(g->usesLineColor() ? g->options.line_width : g->options.gridline_width) = v;
	redraw();
	ut.reg("Change Line Width", [this,v0]{ setLineWidth(v0); }, &(g->usesLineColor() ? g->options.line_width : g->options.gridline_width));
	return true;
}

bool Document::setShinyness(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.shinyness;
	if (fabs(v-v0) < 1e-12) return true;
	g->options.shinyness = v;
	redraw();
	ut.reg("Change Shinyness", [this,v0]{ setShinyness(v0); }, &g->options.shinyness);
	return true;
}

bool Document::setTextureStrength(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.texture_opacity;
	if (fabs(v-v0) < 1e-12) return true;
	g->options.texture_opacity = v;
	redraw();
	ut.reg("Change Texture Strength", [this,v0]{ setTextureStrength(v0); }, &g->options.texture_opacity);
	return true;
}

bool Document::setReflectionStrength(double v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto v0 = g->options.reflection_opacity;
	if (fabs(v-v0) < 1e-12) return true;
	g->options.reflection_opacity = v;
	redraw();
	ut.reg("Change Reflection Strength", [this,v0]{ setReflectionStrength(v0); }, &g->options.reflection_opacity);
	return true;
}

bool Document::setAAMode(AntialiasMode m)
{
	auto m0 = plot.options.aa_mode;
	if (m0 == m) return true;
	plot.options.aa_mode = m;
	redraw();
	ut.reg("Change Antialiasing", [this,m0]{ setAAMode(m0); }, &plot.options.aa_mode);
	return true;
}

bool Document::setTransparencyMode(const GL_BlendMode &m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_BlendMode m0 = g->options.transparency;
	if (m == m0) return true;
	g->options.transparency = m;
	redraw();
	ut.reg("Change Transparency", [this,m0]{ setTransparencyMode(m0); }, &g->options.transparency);
	return true;
}

bool Document::setTextureMode(TextureProjection m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	TextureProjection m0 = g->options.texture_projection;
	if (m == m0) return true;
	g->options.texture_projection = m;
	recalc(g);
	ut.reg("Change Texture Mode", [this,m0]{ setTextureMode(m0); }, &g->options.texture_projection);
	return true;
}

/*void SideSectionStyle::OnCycleTextureMode(int d)
{
	assert(d == 1 || d == -1);
	Graph *g = document().plot.current_graph(); if (!g) return;

	int m = d + (int)g->options.texture_projection;
	if (m < 0) m = TP_LAST;
	if (m > TP_LAST) m = 0;

	g->options.texture_projection = (TextureProjection)m;
	Recalc(g);
	Update(false);
}*/

bool Document::setBgColor(const GL_Color &v)
{
	GL_Color v0 = plot.axis.options.background_color;
	if (v == v0) return true;
	plot.axis.options.background_color = v;
	redraw();
	ut.reg("Change Background Color", [this,v0]{ setBgColor(v0); }, &plot.axis.options.background_color);
	return true;
}

bool Document::setAxisColor(const GL_Color &v)
{
	GL_Color v0 = plot.axis.options.axis_color;
	if (v == v0) return true;
	plot.axis.options.axis_color = v;
	redraw();
	ut.reg("Change Axis Color", [this,v0]{ setAxisColor(v0); }, &plot.axis.options.axis_color);
	return true;
}

bool Document::setFillColor(const GL_Color &v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Color v0 = g->options.fill_color;
	if (v == v0) return true;
	g->options.fill_color = v;
	redraw();
	ut.reg("Change Fill Color", [this,v0]{ setFillColor(v0); }, &g->options.fill_color);
	return true;
}

bool Document::setGridColor(const GL_Color &v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	auto &c = (g->usesLineColor() ? g->options.line_color : g->options.grid_color);
	GL_Color v0 = c;
	if (v == v0) return true;
	c = v;
	redraw();
	ut.reg("Change Grid Color", [this,v0]{ setGridColor(v0); }, &c);
	return true;
}

bool Document::setTexture(GL_ImagePattern v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Image &m0 = g->options.texture;
	GL_ImagePattern v0 = m0.is_pattern();
	if (v0)
	{
		if (v0 == v) return true;
		ut.reg("Change Texture", [this,v0]{ setTexture(v0); }, &m0);
		m0 = v;
	}
	else
	{
		GL_Image tmp; tmp = v; tmp.swap(m0);
		ut.reg("Change Texture", [this,tmp]() mutable { setTexture(tmp); });
	}
	g->isColor() ? recalc(g) : redraw();
	return true;
}
bool Document::setTexture(GL_Image &v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Image &m0 = g->options.texture;
	GL_ImagePattern v0 = m0.is_pattern();
	if (v0)
	{
		ut.reg("Change Texture", [this,v0]{ setTexture(v0); });
		m0.swap(v);
	}
	else
	{
		m0.swap(v);
		ut.reg("Change Texture", [this,v]() mutable{ setTexture(v); });
	}
	g->isColor() ? recalc(g) : redraw();
	return true;
}

bool Document::setReflectionTexture(GL_ImagePattern v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Image &m0 = g->options.reflection_texture;
	GL_ImagePattern v0 = m0.is_pattern();
	if (v0)
	{
		if (v0 == v) return true;
		ut.reg("Change Reflection Texture", [this,v0]{ setReflectionTexture(v0); }, &m0);
		m0 = v;
	}
	else
	{
		GL_Image tmp; tmp = v; tmp.swap(m0);
		ut.reg("Change Reflection Texture", [this,tmp]() mutable { setReflectionTexture(tmp); });
	}
	g->isColor() ? recalc(g) : redraw();
	return true;
}
bool Document::setReflectionTexture(GL_Image &v)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Image &m0 = g->options.reflection_texture;
	GL_ImagePattern v0 = m0.is_pattern();
	if (v0)
	{
		ut.reg("Change Reflection Texture", [this,v0]{ setReflectionTexture(v0); });
		m0.swap(v);
	}
	else
	{
		m0.swap(v);
		ut.reg("Change Reflection Texture", [this,v]() mutable{ setReflectionTexture(v); });
	}
	g->isColor() ? recalc(g) : redraw();
	return true;
}

bool Document::loadTexture(const std::string &path)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Image m; if (!m.load(path)) return false;
	GL_Image &m0 = g->options.texture;
	GL_ImagePattern v0 = m0.is_pattern();
	if (v0)
	{
		ut.reg("Change Texture", [this,v0]{ setTexture(v0); });
		m0.swap(m);
	}
	else
	{
		m0.swap(m);
		ut.reg("Change Texture", [this,m]() mutable{ setTexture(m); });
	}
	g->isColor() ? recalc(g) : redraw();
	return true;
}
bool Document::loadReflectionTexture(const std::string &path)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GL_Image m; if (!m.load(path)) return false;
	GL_Image &m0 = g->options.reflection_texture;
	GL_ImagePattern v0 = m0.is_pattern();
	if (v0)
	{
		ut.reg("Change Reflection Texture", [this,v0]{ setReflectionTexture(v0); });
		m0.swap(m);
	}
	else
	{
		m0.swap(m);
		ut.reg("Change Reflection Texture", [this,m]() mutable{ setReflectionTexture(m); });
	}
	g->isColor() ? recalc(g) : redraw();
	return true;
}

bool Document::setAxisFont(const std::string &name, float size)
{
	GL_Font &f = plot.axis.options.label_font;
	const std::string n0 = f.name;
	float s0 = f.size;
	if (n0 == name && fabs(s0-size) < 1e-8) return true;
	ut.reg("Change Axis Font", [this,n0,s0]{ setAxisFont(n0,s0); }, &f);
	f.name = name;
	f.size = size;
	redraw();
	return true;
}

//-------------------------------------------------------------

bool Document::setF(int i, const std::string &s)
{
	if (i < 1 || i > 3) { assert(false); return false; }
	Graph *g = plot.current_graph(); if (!g) return false;
	std::string s0 = g->fn(i);
	if (s == s0) return true;
	ut.reg("Change Function", [this,i,s0]{ setF(i,s0); }, g, i);
	g->set(s, i);
	redraw();
	return true;
}

bool Document::setDomain(GraphType t)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GraphType t0 = g->type();
	if (t == t0) return true;
	ut.reg("Change Domain", [this,t0]{ setDomain(t0); }, g, 4);
	g->type(t);
	plot.update_axis();
	redraw();
	return true;
}

bool Document::setCoords(GraphCoords c)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GraphCoords c0 = g->coords();
	if (c == c0) return true;
	ut.reg("Change Coordinates", [this,c0]{ setCoords(c0); }, g, 5);
	g->coords(c);
	redraw();
	return true;
}

bool Document::setMode(GraphMode m)
{
	Graph *g = plot.current_graph(); if (!g) return false;
	GraphMode m0 = g->mode();
	if (m == m0) return true;
	ut.reg("Change Graph Mode", [this,m0]{ setMode(m0); }, g, 6);
	g->mode(m);
	plot.update_axis();
	redraw();
	return true;
}

bool Document::deleteGraph(IDCarrier::OID g_, IDCarrier::OID g_sel)
{
	Graph *g = (Graph*)IDCarrier::find(g_); if (!g) return false;
	const bool cur = (plot.current_graph() == g);

	std::vector<char> data;
	ArrayWriter w(data);
	Serializer s(w);
	g->save(s);

	ut.reg("Delete Graph", [this,data,g_,cur]{ undeleteGraph(data, g_, cur); });

	plot.delete_graph(g); g = NULL;
	if (g_sel)
	{
		Graph *g0 = (Graph*)IDCarrier::find(g_sel);
		if (g0) plot.set_current_graph(g0);
	}

	plot.update_axis();
	redraw();
	return true;
}
bool Document::undeleteGraph(const std::vector<char> &data, IDCarrier::OID g_, bool make_current)
{
	ArrayReader w(data);
	Deserializer s(w);
	Graph *g = new Graph(plot);
	g->load(s);
	g->restore(g_);
	plot.add_graph(g, make_current);
	plot.update_axis();
	redraw();
	ut.reg("Restore Graph", [this,g_]{ deleteGraph(g_); });
	return true;
}

bool Document::addGraph()
{
	Graph *g0 = plot.current_graph();
	Graph *g = plot.add_graph();
	IDCarrier::OID g_ = g->oid(), g0_ = (g0 ? g0->oid() : 0);
	plot.update_axis();
	redraw();
	ut.reg("Add Graph", [this,g_,g0_]{ deleteGraph(g_, g0_); });
	return true;
}

bool Document::selectGraph(int i)
{
	int i0 = plot.current_graph_index();
	if (i == i0) return true;
	ut.reg("Select Graph", [this,i0]{ selectGraph(i0); }, &plot, 1);
	plot.set_current_graph(i);
	return true;
}

bool Document::toggleGraphVisibility()
{
	Graph *g = plot.current_graph(); if (!g) return false;
	g->options.hidden = !g->options.hidden;
	plot.update_axis();
	recalc(g);
	ut.reg("Change Graph Visibility", [this]{ toggleGraphVisibility(); }, &g->options.hidden, TOGGLE_OP);
	return true;
}

//-------------------------------------------------------------

bool Document::deleteParam(IDCarrier::OID p_)
{
	Parameter *p = (Parameter*)IDCarrier::find(p_); if (!p) return false;
	std::vector<char> data;
	ArrayWriter w(data);
	Serializer s(w);
	p->save(s);
	ut.reg("Delete Parameter", [this,data,p_]{ undeleteParam(data, p_); });
	std::string nm = p->name();
	delete p; p = NULL;
	plot.reparse(nm);
	recalc(plot);
	return true;
}

bool Document::undeleteParam(const std::vector<char> &data, IDCarrier::OID g_)
{
	ArrayReader w(data);
	Deserializer s(w);
	Parameter *p = new Parameter;
	p->load(s);
	p->restore(g_);
	plot.ns.add(p);
	plot.reparse(p->name());
	recalc(plot);
	ut.reg("Restore Parameter", [this,g_]{ deleteParam(g_); });
	return true;
}

bool Document::modifyParam(const std::vector<char> &data, IDCarrier::OID p_)
{
	Parameter *p = (Parameter*)IDCarrier::find(p_); if (!p) return false;

	std::vector<char> data2;
	ArrayWriter w(data2);
	Serializer s(w);
	p->save(s);

	ArrayReader ww(data);
	Deserializer d(ww);
	p->load(d);

	plot.reparse(p->name());
	recalc(plot);
	ut.reg("Modify Parameter", [this,data2,p_]{ modifyParam(data2, p_); });
	return true;
}

bool Document::deleteDef(IDCarrier::OID p_)
{
	UserFunction *p = (UserFunction*)IDCarrier::find(p_); if (!p) return false;
	std::vector<char> data;
	ArrayWriter w(data);
	Serializer s(w);
	p->save(s);
	ut.reg("Delete Definition", [this,data,p_]{ undeleteDef(data, p_); });
	std::string nm = p->name();
	delete p; p = NULL;
	plot.reparse(nm);
	recalc(plot);
	return true;
}

bool Document::undeleteDef(const std::vector<char> &data, IDCarrier::OID g_)
{
	ArrayReader w(data);
	Deserializer s(w);
	UserFunction *p = new UserFunction;
	p->load(s);
	p->restore(g_);
	plot.ns.add(p);
	plot.reparse(p->name());
	recalc(plot);
	ut.reg("Restore Definition", [this,g_]{ deleteDef(g_); });
	return true;
}

bool Document::modifyDef(const std::vector<char> &data, IDCarrier::OID p_)
{
	UserFunction *p = (UserFunction*)IDCarrier::find(p_); if (!p) return false;

	std::vector<char> data2;
	ArrayWriter w(data2);
	Serializer s(w);
	p->save(s);

	ArrayReader ww(data);
	Deserializer d(ww);
	p->load(d);

	plot.reparse(p->name());
	recalc(plot);
	ut.reg("Modify Definition", [this,data2,p_]{ modifyDef(data2, p_); });
	return true;
}

//-------------------------------------------------------------

void Document::undoForCam()
{
	Camera &cam = plot.camera;
	Quaternion q0 = cam.quat();
	double     z0 = cam.zoom();
	ut.reg("Change View", [this,q0,z0]{ setCam(q0,z0); }, &cam, 1, true);
}
bool Document::setCam(const Quaternion &rot, double zoom)
{
	Camera &cam = plot.camera;
	Quaternion q0 = cam.quat();
	double     z0 = cam.zoom();
	ut.reg("Change View", [this,q0,z0]{ setCam(q0,z0); }, &cam, 1, true);
	cam.quat(rot);
	cam.set_zoom(zoom);
	return true;
}
void Document::undoForAxis()
{
	Axis &ax = plot.axis;
	P3d c0, r0; ax.get_range(c0, r0);
	ut.reg("Change Axis Range", [this,c0,r0]{ setAxis(c0,r0); }, &ax, 1, true);
}
bool Document::setAxis(const P3d &center, const P3d &range)
{
	undoForAxis();
	plot.axis.set_range(center, range);
	recalc(plot);
	return true;
}
void Document::undoForInRange()
{
	Axis &ax = plot.axis;
	P2d c0, r0; ax.get_inrange(c0, r0);
	ut.reg("Change Axis Range", [this,c0,r0]{ setInRange(c0,r0); }, &ax, 2, true);
}
bool Document::setInRange(const P2d &center, const P2d &range)
{
	undoForInRange();
	plot.axis.set_inrange(center, range);
	recalc(plot);
	return true;
}

#if 0

#endif
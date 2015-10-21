#include "Graph.h"
#include "Plot.h"
#include "../Engine/Namespace/Namespace.h"
#include "../Engine/Namespace/Expression.h"
#include "../Engine/Namespace/Variable.h"
#include "../Engine/Namespace/AliasVariable.h"
#include "../Engine/Parser/Evaluator.h"
#include "../Engine/Namespace/Parameter.h"
#include "Graphics/GL_Graph.h"
#include "Graphics/GL_AreaGraph.h"
#include "Graphics/GL_ImplicitAreaGraph.h"
#include "Graphics/GL_ImplicitLineGraph.h"
#include "Graphics/GL_LineGraph.h"
#include "Graphics/GL_PointGraph.h"
#include "Graphics/GL_ColorGraph.h"
#include "Graphics/GL_RiemannColorGraph.h"
#include "Graphics/GL_Histogram.h"
#include "Graphics/GL_HistogramPointGraph.h"
#include "Graphics/GL_RiemannHistogram.h"
#include "Graphics/GL_RiemannHistogramPointGraph.h"

void GraphOptions::save(Serializer &s) const
{
	s._bool(hidden);
	fill_color.save(s);
	grid_color.save(s);
	line_color.save(s);
	s._bool(clip_graph_to_axis);
	s._bool(clip_parametric_to_axis);
	texture.save(s);
	reflection_texture.save(s);
	mask.save(s);
	s._double(texture_opacity);
	s._double(reflection_opacity);
	s._double(shinyness);
	s._double(line_width);
	s._double(gridline_width);
	s._double(vf_scale);
	s._enum(grid_style, Grid_Off, Grid_Full);
	s._double(grid_density);
	s._enum(shading_mode, Shading_Points, Shading_Smooth);
	s._enum(vf_mode, VF_Unscaled, VF_Connected);
	transparency.save(s);
	s._double(quality);
	if (s.version() >= FILE_VERSION_1_2) s._bool(disco);
	
	if (s.version() >= FILE_VERSION_1_6)
	{
		s._enum(texture_projection, TP_Repeat, TP_UV);
	}
	else if (s.version() >= FILE_VERSION_1_4)
	{
		if (texture_projection != TP_Riemann && texture_projection != TP_Repeat)
		{
			NEEDS_VERSION(FILE_VERSION_1_6, "texture coordinates");
		}
		s._bool(texture_projection == TP_Riemann);
	}
	
	if (s.version() >= FILE_VERSION_1_9)
	{
		s._enum(hist_mode, HM_Riemann, HM_Normal);
		s._double(hist_scale);
	}
}
void GraphOptions::load(Deserializer &s)
{
	s._bool(hidden);
	fill_color.load(s);
	grid_color.load(s);
	line_color.load(s);
	s._bool(clip_graph_to_axis);
	s._bool(clip_parametric_to_axis);
	texture.load(s);
	reflection_texture.load(s);
	mask.load(s);
	s._double(texture_opacity);
	s._double(reflection_opacity);
	s._double(shinyness);
	s._double(line_width);
	s._double(gridline_width);
	s._double(vf_scale);
	s._enum(grid_style, Grid_Off, Grid_Full);
	s._double(grid_density);
	s._enum(shading_mode, Shading_Points, Shading_Smooth);
	s._enum(vf_mode, VF_Unscaled, VF_Connected);
	
	transparency.load(s);
	if (s.version() < FILE_VERSION_1_7 && fill_color.opaque()) transparency.off = true;
	
	s._double(quality);
	if (s.version() >= FILE_VERSION_1_2) s._bool(disco); else disco = false;
	
	if (s.version() >= FILE_VERSION_1_6)
	{
		s._enum(texture_projection, TP_Repeat, TP_UV);
	}
	else if (s.version() >= FILE_VERSION_1_4)
	{
		bool riemann_color;
		s._bool(riemann_color);
		texture_projection = (riemann_color ? TP_Riemann : TP_Repeat);
	}
	else
	{
		texture_projection = TP_Repeat;
	}
	
	if (s.version() >= FILE_VERSION_1_9)
	{
		s._enum(hist_mode, HM_Riemann, HM_Normal);
		s._double(hist_scale);
	}
	else
	{
		hist_mode  = HM_Riemann;
		hist_scale = 1.0;
	}
}

Graph::Graph(Plot &p) : plot(p), gl(NULL), ex(NULL), m_type(R_R), m_coords(GC_Cartesian), m_mode(GM_Graph), m_gl_class(-1), m_need_update(true)
{
	ins.link(&p.ns);
}

Graph::Graph(const Graph &g)
: plot(g.plot)
, options(g.options)
, m_f1(g.m_f1), m_f2(g.m_f2), m_f3(g.m_f3)
, m_type(g.m_type)
, m_coords(g.m_coords)
, m_mode(g.m_mode)
, ex(NULL), gl(NULL), m_gl_class(-1), m_need_update(true)
{
	ins.link(&plot.ns);
	invalidate();
}

Graph::Graph(const Graph &g, Plot &plot)
: plot(plot)
, options(g.options)
, m_f1(g.m_f1), m_f2(g.m_f2), m_f3(g.m_f3)
, m_type(g.m_type)
, m_coords(g.m_coords)
, m_mode(g.m_mode)
, ex(NULL), gl(NULL), m_gl_class(-1), m_need_update(true)
{
	ins.link(&plot.ns);
	invalidate();
}

Graph::~Graph()
{
	plot.ns.remove(&ins);
	delete ex;
	delete gl;
}

void Graph::save(Serializer &s) const
{
	if (m_mode == GM_Color) CHECK_VERSION(FILE_VERSION_1_4, "color mode graphs");
	if (m_mode == GM_RiemannColor) CHECK_VERSION(FILE_VERSION_1_6, "riemann color mode graphs");
	if (m_mode == GM_Histogram) CHECK_VERSION(FILE_VERSION_1_8, "histogram graphs");

	s._string(m_f1);
	s._string(m_f2);
	s._string(m_f3);
	s._enum(m_type, R_R, R3_R3);
	s._enum(m_mode, GM_Graph, GM_Histogram);
	s._enum(m_coords, GC_Cartesian, GC_Cylindrical);
	options.save(s);
}
void Graph::load(Deserializer &s)
{
	s._string(m_f1);
	s._string(m_f2);
	s._string(m_f3);
	s._enum(m_type, R_R, R3_R3);
	s._enum(m_mode, GM_Graph,
			s.version() < FILE_VERSION_1_4 ? GM_Implicit :
			s.version() < FILE_VERSION_1_6 ? GM_Color :
			s.version() < FILE_VERSION_1_8 ? GM_RiemannColor : GM_Histogram);
	s._enum(m_coords, GC_Cartesian, GC_Cylindrical);
	options.load(s);
	invalidate();
}

void Graph::invalidate()
{
	update(CH_UNKNOWN);
	delete ex; ex = NULL;
	delete gl; gl = NULL;
	
	vars.clear();
	ins.clear();
	Variable *x, *y, *z;
	Expression xx; ins.add(&xx);
	
	#define  ADD(name, arg) if (!ins.find(name,-1) && !ins.find(name,0)) ins.add(new AliasVariable(name, arg));
	#define ADDP(name, arg) if (!ins.find(name,-1) && !ins.find(name,0)){ xx.strings(arg); ins.add(new AliasVariable(name, xx)); }

	switch (m_type)
	{
		case  R_R:
		case  R_R2:
		case  R_R3:
		case S1_R2:
		case S1_R3:
		{
			x = new Variable("x", true); ins.add(x); vars.push_back(x);
			ADD("t", x);
			ADDP("y", "0");
			ADD("z", x);
			ADD("u", x);
			ADDP("v", "0");
			ADDP("r", "abs(x)");
			break;
		}
		
		case R2_R:
		case R2_R3:
		case S2_R3:
		case R2_R2:
		{
			x = new Variable("x", true); ins.add(x); vars.push_back(x);
			y = new Variable("y", true); ins.add(y); vars.push_back(y);
			ADD("u", x);
			ADD("v", y);
			ADDP("z", "0");
			ADDP("r", "hypot(x,y)");
			ADDP("phi", "arg(x,y)");
			break;
		}
			
		case  C_C:
		{
			z = new Variable("z", false); ins.add(z); vars.push_back(z);
			ADDP("x",   "re(z)"); ADDP("u", "re(z)");
			ADDP("y",   "im(z)"); ADDP("v", "im(z)");
			ADDP("r",   "abs(z)");
			ADDP("phi", "arg(z)");
			break;
		}
		
		case R3_R:
		case R3_R3:
		{
			x = new Variable("x", true); ins.add(x); vars.push_back(x);
			y = new Variable("y", true); ins.add(y); vars.push_back(y);
			z = new Variable("z", true); ins.add(z); vars.push_back(z);
			ADD("u", x);
			ADD("v", y);
			ADDP("r",     "__rsqrt__(x²+y²+z²)");
			ADDP("phi",   "arg(x,y)");
			ADDP("theta", "arg(z,__rsqrt__(x²+y²))");
			break;
		}
	}
	ins.remove(&xx);
	
	#undef ADD
	#undef ADDP
}

void Graph::swap_fields(int i, int j)
{
	if (i == j || i < 1 || j < 1 || i > 3 || j > 3){ assert(false); return; }
	if (i > j) std::swap(i,j);
	if (i == 1 && j == 2) set(m_f2, m_f1, m_f3);
	else if (i == 2 && j == 3) set(m_f1, m_f3, m_f2);
	else if (i == 1 && j == 3) set(m_f3, m_f2, m_f1);
}

void Graph::set(std::string f1, std::string f2, std::string f3)
{
	if (f1 == m_f1 && f2 == m_f2 && f3 == m_f3) return;
	m_f1 = f1;
	m_f2 = f2;
	m_f3 = f3;
	invalidate();
}

void Graph::type(GraphType t)
{
	if (t == m_type) return;
	m_type = t;
	fix_mode();
	fix_coords();
	invalidate();
}
void Graph::mode(GraphMode m)
{
	if (m == m_mode) return;
	m_mode = m;
	fix_mode();
	invalidate();
}
void Graph::coords(GraphCoords c)
{
	if (c == m_coords) return;
	m_coords = c;
	fix_coords();
	invalidate();
}

bool Graph::isLine() const
{
	switch (m_type)
	{
		case  R_R:
		case  R_R2:
		case  R_R3:
		case S1_R2:
		case S1_R3:
			return true;
			
		case R2_R:
			return isImplicit();
			
		case R2_R3:
		case S2_R3:
		case R2_R2:
		case  C_C:
		case R3_R3:
		case R3_R:
			return false;
	}
	assert(false); throw std::logic_error("type corrupted");
}

bool Graph::wantUprightView() const
{
	switch (m_type)
	{
		case  R_R:
		case  R_R2:
		case S1_R2:
		case R2_R2:
			return true; // 2D

		case R2_R:
		case  C_C:
			return true; // for both Riemann and graphs

		case R3_R:
		case R3_R3:
			return true; // vector field

		case  R_R3:
		case S1_R3:
		case R2_R3:
		case S2_R3:
			return false; // parametrics
	}
	assert(false); throw std::logic_error("type corrupted");
}
bool Graph::isParametric() const
{
	switch (m_type)
	{
		case  R_R:
		case R2_R:
		case R3_R3:
		case R3_R:
			return false;
		
		case  C_C:
			return m_mode == GM_Riemann || m_mode == GM_Image;
		
		case R2_R2:
			return m_mode == GM_Image;
		
		case  R_R2:
		case S1_R2:
		case  R_R3:
		case S1_R3:
		case R2_R3:
		case S2_R3:
			return true; // parametrics
	}
	assert(false); throw std::logic_error("type corrupted");
}

Axis::Type Graph::axisType() const
{
	switch (m_type)
	{
		case  R_R:
		case  R_R2:
		case S1_R2:
		case R2_R2:
			return Axis::Rect;

		case R2_R:
			return m_mode == GM_Implicit ? Axis::Rect : Axis::Box;
			
		case  R_R3:
		case S1_R3:
		case R2_R3:
		case S2_R3:
		case R3_R3:
		case R3_R:
			return Axis::Box;
			
		case C_C:  return (m_mode==GM_Riemann || m_mode==GM_RiemannColor ||
		                   m_mode==GM_Histogram && options.hist_mode == HM_Riemann) ? Axis::Sphere :
						  (m_mode==GM_Image   || m_mode==GM_Color || m_mode==GM_Histogram && options.shading_mode == Shading_Points) ? Axis::Rect :
						   Axis::Box;
	}
	return Axis::Invalid;
}

int Graph::inRangeDimension() const
{
	// for parametrics
	switch (m_type)
	{
		case  R_R:
		case S1_R2:
		case S1_R3:
		case S2_R3:
		case R2_R:
		case R3_R3:
		case R3_R:
			return 0;

		case R2_R2:
			return m_mode==GM_Image ? 2 : 0;
		case R2_R3:
			return 2;

		case  R_R2:
		case  R_R3:
			return 1;
			
		case C_C:
			return (m_mode==GM_Riemann || m_mode==GM_Image) ? 2 : 0;
	}
	return 0;
}


Expression *Graph::expression() const
{
	if (!ex)
	{
		std::vector<std::string> strings;
		switch (m_type)
		{
			case  R_R: 
			case R2_R: 
			case R3_R:
			case  C_C:
				strings.push_back(m_f1);
				break;
				
			case  R_R2:
			case R2_R2:
			case S1_R2:
				strings.push_back(m_f1);
				strings.push_back(m_f2);
				break;
				
			case  R_R3:
			case R2_R3:
			case S2_R3:
			case R3_R3:
			case S1_R3:
				strings.push_back(m_f1);
				strings.push_back(m_f2);
				strings.push_back(m_f3);
				break;
		}
		ex = new Expression;
		ins.add(ex);
		ex->strings(strings);
	}
	return ex;
}

Evaluator *Graph::evaluator() const
{
	if (!ex)
	{
		expression();
		if (!ex || !ex->valid()) return NULL;
	}
	Evaluator *e = ex->evaluator(vars);
	if (e) e->set_parameters(used_parameters());
	return e;
}

bool Graph::isValid() const
{
	if (!ex)
	{
		expression();
		if (!ex) return false;
	}
	return ex->valid();
}

std::set<Parameter*> Graph::used_parameters() const
{
	if (!ex)
	{
		expression();
		if (!ex || !ex->valid()) return std::set<Parameter*>();
	}
	return ex->usedParameters();
}
bool Graph::uses_parameter(const Parameter &p) const
{
	if (!ex)
	{
		expression();
		if (!ex || !ex->valid()) return false;
	}
	return ex->usedParameters().count(const_cast<Parameter*>(&p));
}
bool Graph::uses_object(const std::string &name) const
{
	if (!ex)
	{
		expression();
		if (!ex || !ex->valid()) return false;
	}
	return ex->uses_object(name);
}

void Graph::update(ChangeType t)
{
	//if (t == CH_AXIS_RANGE && isParametric() && !clipping()) return;
	if (t == CH_IN_RANGE && !isParametric()) return;
	m_need_update = true;
	if (t == CH_EXPRESSION) invalidate();
}

std::vector<std::string> Graph::components() const
{
	std::vector<std::string> v;
	switch (m_type)
	{
		case  R_R: v.push_back("y"); break;
		case R2_R: v.push_back("z"); break;
		case  C_C: v.push_back("w"); break;
		case R3_R: v.push_back("f"); break;
			
		case  R_R2:
		case S1_R2:
		case R2_R2:
			if (m_coords == GC_Cartesian)
			{
				v.push_back("x");
				v.push_back("y");
			}
			else
			{
				assert(m_coords == GC_Polar);
				v.push_back("r");
				v.push_back("φ");
			}
			break;
			
		case  R_R3:
		case S1_R3:
		case R2_R3:
		case S2_R3:
		case R3_R3:
			if (m_coords == GC_Cartesian)
			{
				v.push_back("x");
				v.push_back("y");
				v.push_back("z");
			}
			else if (m_coords == GC_Spherical)
			{
				v.push_back("r");
				v.push_back("φ");
				v.push_back("ϑ");
			}
			else
			{
				assert(m_coords == GC_Cylindrical);
				v.push_back("r");
				v.push_back("φ");
				v.push_back("z");
			}
			break;
	}
	return v;
}

int Graph::n_components() const
{
	switch (m_type)
	{
		case  R_R:
		case R2_R:
		case R3_R:
		case  C_C: return 1;
			
		case  R_R2:
		case S1_R2:
		case R2_R2: return 2;
			
		case  R_R3:
		case S1_R3:
		case R2_R3:
		case S2_R3:
		case R3_R3: return 3;
	}
	assert(false); throw std::logic_error("type corrupted");
}

std::string Graph::description_line() const
{
	int n = n_components();
	std::string d;
	for (int i = 0; i < n; ++i)
	{
		if (i > 0) d += "; ";
		const std::string &fi = (i==0 ? m_f1 : i==1 ? m_f2 : m_f3);
		d += fi.empty() ? "<empty>" : fi;
	}
	return d;
}

std::vector<GraphMode> Graph::valid_modes() const
{
	std::vector<GraphMode> v;
	switch (m_type)
	{
		case R_R:
			v.push_back(GM_Graph);
			break;
			
		case R2_R:
			v.push_back(GM_Graph);
			v.push_back(GM_Implicit);
			break;

		case R3_R:
			v.push_back(GM_Implicit);
			break;
			
		case C_C:
			v.push_back(GM_Re);
			v.push_back(GM_Im);
			v.push_back(GM_Abs);
			v.push_back(GM_Phase);
			v.push_back(GM_Image);
			v.push_back(GM_Riemann);
			v.push_back(GM_Color);
			v.push_back(GM_RiemannColor);
			v.push_back(GM_Histogram);
			break;
			
		case  R_R2:
		case S1_R2:
			v.push_back(GM_Image);
			break;

		case R2_R2:
			v.push_back(GM_Image);
			v.push_back(GM_VF);
			v.push_back(GM_Color);
			break;
			
		case  R_R3:
		case S1_R3:
			v.push_back(GM_Image);
			break;

		case R2_R3:
		case S2_R3:
			v.push_back(GM_Image);
			break;

		case R3_R3:
			v.push_back(GM_VF);
			break;
	}
	return v;
}
std::vector<GraphCoords> Graph::valid_coords() const
{
	std::vector<GraphCoords> v;
	v.push_back(GC_Cartesian);
	switch (m_type)
	{
		case R_R:
		case R2_R:
		case R3_R:
		case C_C: break;
			
		case R_R2:
		case S1_R2:
		case R2_R2: v.push_back(GC_Polar); break;

		case R_R3:
		case S1_R3:
		case R2_R3:
		case S2_R3:
		case R3_R3: v.push_back(GC_Spherical); v.push_back(GC_Cylindrical); break;
	}
	return v;
}

void Graph::fix_mode()
{
	auto v = valid_modes();
	assert(!v.empty());
	if (std::find(v.begin(), v.end(), m_mode) == v.end())
	{
		m_mode = v[0];
	}
}
void Graph::fix_coords()
{
	auto v = valid_coords();
	assert(!v.empty());
	if (std::find(v.begin(), v.end(), m_coords) == v.end())
	{
		m_coords = v[0];
	}
}

GL_Graph *Graph::gl_graph() const
{
	enum GraphTypes
	{
		GC_Invalid = -1,
		GC_PointGraph = 1,
		GC_LineGraph,
		GC_ImplicitLineGraph,
		GC_AreaGraph, 
		GC_ImplicitAreaGraph,
		GC_ColorGraph,
		GC_RiemannColorGraph,
		GC_Histogram,
		GC_HistogramPointGraph,
		GC_RiemannHistogram,
		GC_RiemannHistogramPointGraph
	};
	
	#define UPDATE(X) \
	do if (!gl || m_gl_class != GC_##X){ \
		delete gl; gl = NULL; \
		m_gl_class = GC_##X; gl = new GL_##X(*const_cast<Graph*>(this)); \
	} while(0)
	
	expression();
	if (!ex || !ex->valid())
	{
		m_gl_class = GC_Invalid;
		delete gl; gl = NULL;
	}
	else if (isLine() && isImplicit())
	{
		UPDATE(ImplicitLineGraph);
	}
	else if (isLine())
	{
		UPDATE(LineGraph);
	}
	else if (isColor() && m_mode == GM_RiemannColor)
	{
		UPDATE(RiemannColorGraph);
	}
	else if (isColor())
	{
		UPDATE(ColorGraph);
	}
	else if (isHistogram())
	{
		if (options.hist_mode != HM_Riemann)
		{
			if (options.shading_mode == Shading_Points)
				UPDATE(HistogramPointGraph);
			else
				UPDATE(Histogram);
		}
		else
		{
			if (options.shading_mode == Shading_Points)
				UPDATE(RiemannHistogramPointGraph);
			else
				UPDATE(RiemannHistogram);
		}
	}
	else if (isVectorField() || isArea() && options.shading_mode == Shading_Points)
	{
		UPDATE(PointGraph);
	}
	else if (m_type == R3_R /* && !Shading_Points */)
	{
		UPDATE(ImplicitAreaGraph);
	}
	else
	{
		UPDATE(AreaGraph);
	}
	return gl;
}


#pragma once
#include "../Engine/Namespace/Namespace.h"
#include "Geometry/Axis.h"
#include "OpenGL/GL_Color.h"
#include "OpenGL/GL_BlendMode.h"
#include "OpenGL/GL_Image.h"
#include "OpenGL/GL_Mask.h"
#include "../Engine/Namespace/ObjectDB.h"
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <stdexcept>
class Expression;
class Evaluator;
class Variable;
class Parameter;
class GL_Graph;
struct Plot;

enum GraphType
{
	// graphs
	R_R  = 0,
	R2_R = 1,
	C_C  = 2,
	
	// 1d parametrics
	R_R2  = 4,
	R_R3  = 5,
	S1_R2 = 6,
	S1_R3 = 7,
	
	// 2d parametrics
	R2_R3 = 9,
	S2_R3 = 10,
	
	// implicit
	R3_R  = 12,
	// vector fields
	R2_R2 = 13,
	R3_R3 = 14,
};

enum GraphCoords
{
	GC_Cartesian   =  0,
	GC_Polar       =  1,
	GC_Spherical   =  2,
	GC_Cylindrical =  3
};

enum GraphMode
{
	GM_Graph        =  0,
	GM_Image        =  1,
	GM_Riemann      =  2,
	GM_VF           =  3,
	GM_Re           =  4,
	GM_Im           =  5,
	GM_Abs          =  6,
	GM_Phase        =  7,
	GM_Implicit     =  8,
	GM_Color        =  9,
	GM_RiemannColor = 10,
	GM_Histogram    = 11
};

enum HistogramMode
{
	HM_Riemann = 0,
	HM_Disc    = 1,
	HM_Normal  = 2
};

enum VectorfieldMode
{
	VF_Unscaled   = 0,
	VF_Normalized = 1,
	VF_Unit       = 2,
	VF_Connected  = 3,
	VF_LAST       = VF_Connected
};

enum GridStyle
{
	Grid_Off  = 0,
	Grid_On   = 1,
	Grid_Full = 2
};

enum ShadingMode
{
	Shading_Points     = 0,
	Shading_Wireframe  = 1,
	Shading_Hiddenline = 2,
	Shading_Flat       = 3,
	Shading_Smooth     = 4
};

enum TextureProjection
{
	TP_Repeat  = 0,
	TP_Center  = 1,
	TP_Riemann = 2,
	TP_UV      = 3,
	TP_LAST    = TP_UV
};

enum
{
	CH_AXIS_RANGE =  1,
	CH_IN_RANGE   =  2,
	CH_PARAM      =  4,
	CH_AXIS_TYPE  =  8,
	CH_EXPRESSION = 16,
	CH_UNKNOWN    = 0xFFFF
};
typedef int ChangeType;

struct GraphOptions : public Serializable
{
	GraphOptions()
	: fill_color(0.25f, 0.8f, 1.0f), grid_color(1.0f, 0.1f), line_color(0.1f)
	, hidden(false)
	, clip_graph_to_axis(true), clip_parametric_to_axis(false)
	, texture_opacity(0.0), reflection_opacity(0.0)
	, shinyness(0.5)
	, line_width(1.5), gridline_width(1.0)
	, grid_style(Grid_On), grid_density(16.0)
	, shading_mode(Shading_Smooth)
	, vf_mode(VF_Normalized), vf_scale(1.0)
	, hist_mode(HM_Riemann), hist_scale(1.0)
	, quality(0.25)
	, disco(false)
	, texture_projection(TP_Repeat)
	{ }
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	GL_Color fill_color, grid_color, line_color;
	GL_BlendMode transparency;
	
	bool     hidden;
	bool     clip_graph_to_axis;      // only relevant for Axis::Box
	bool     clip_parametric_to_axis; // only relevant for Axis::Box, for parametrics (including GM_Image and GM_Riemann)
	GL_Image texture, reflection_texture;
	GL_Mask  mask;
	double   texture_opacity;       // 0 (no texture) .. 1 (full texturing, fill irrelevant)
	double   reflection_opacity;    // 0..1
	double   shinyness;             // 0..1
	double   line_width;            // in pixels - for linegraphs, Shading_Points, Shading_Wireframe
	double   gridline_width;        // in pixels - grid
	double   vf_scale;              // 0..inf (multiplier for vf or vf^0)
	double   hist_scale;            // 0..inf (radius of disc / sigma)
	//double   vf_density;          // 0..1
	double   quality;               // 0..1
	bool     disco;                 // detect discontinuities?

	TextureProjection texture_projection;

	GridStyle grid_style;
	double    grid_density; // 0..1 (0: maximum spacing, 1: 10px spacing)
	
	ShadingMode     shading_mode;
	VectorfieldMode vf_mode;
	HistogramMode   hist_mode;
};

class Graph : public Serializable, public IDCarrier
{
public:
	Graph(Plot &plot);
	virtual ~Graph();

	Graph(const Graph &g);
	Graph(const Graph &g, Plot &plot); // copy into another namespace
	Graph &operator=(const Graph &) = delete;
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);

	const std::string &f1() const{ return m_f1; }
	const std::string &f2() const{ return m_f2; }
	const std::string &f3() const{ return m_f3; }
	const std::string &fn(int i) const
	{
		assert(i >= 1 && i <= 3);
		return i == 1 ? m_f1 : i == 2 ? m_f2 : m_f3;
	}
	void f1(const std::string &f){ set(   f, m_f2, m_f3); }
	void f2(const std::string &f){ set(m_f1,    f, m_f3); }
	void f3(const std::string &f){ set(m_f1, m_f2,    f); }
	void set(const std::string &f, int i)
	{
		if (i == 1) f1(f);
		else if (i == 2) f2(f);
		else if (i == 3) f3(f);
	}
	void set(std::string f1, std::string f2, std::string f3);
	bool init(bool all, int type, int coords, int mode, int nf, const std::string &f1, const std::string &f2, const std::string &f3);
	void swap_fields(int i, int j);
	const GL_Color &text_color() const; // for graphsBox
	
	std::vector<std::string> components() const; // labels for the f1..f3 fields
	std::vector<GraphMode>   valid_modes() const; // for current GraphType
	std::vector<GraphCoords> valid_coords() const; // for current GraphType
	int n_components() const; // how many of the f1..f3 are used?

	Namespace &internal_namespace() const{ return ins; } // for autocompletion and derivatives

	GraphType     type() const{ return m_type; }
	GraphMode     mode() const{ return m_mode; }
	GraphCoords coords() const{ return m_coords; }
	void       type(GraphType t);
	void       mode(GraphMode t);
	void     coords(GraphCoords c);
	bool clipping() const
	{
		return (isHistogram() || isVectorField() || isImplicit() || isColor()) ? false :
		isParametric() ? options.clip_parametric_to_axis : options.clip_graph_to_axis;
	}
	void clipping(bool c){ (isParametric() ? options.clip_parametric_to_axis : options.clip_graph_to_axis) = c; }
	bool default_clipping() const{ return !isRiemann() && !isParametric(); }
	bool toggle_clipping()
	{
		if (isHistogram() || isVectorField() || isImplicit() || isColor()) return false;
		if (isParametric())
			options.clip_parametric_to_axis = !options.clip_parametric_to_axis;
		else
			options.clip_graph_to_axis = !options.clip_graph_to_axis;
		update(CH_UNKNOWN);
		return true;
	}
	bool toggle_grid(bool full)
	{
		if (isVectorField() || isColor() || !hasFill()) return false;
		auto &m = options.grid_style, n = (full ? Grid_Full : Grid_Off);
		m = (m == n ? Grid_On : n);
		update(CH_UNKNOWN);
		return true;
	}
	
	bool isValid         () const;
	bool isLine          () const; // 1d line graph?
	bool isParametric    () const;
	bool isVectorField   () const{ return m_mode == GM_VF; }
	bool isArea          () const{ return !isLine() && !isVectorField(); }
	bool isImplicit      () const{ return m_mode == GM_Implicit; }
	bool isRiemann       () const{ return m_type == C_C && (m_mode == GM_Riemann || m_mode == GM_RiemannColor || m_mode == GM_Histogram && options.hist_mode == HM_Riemann); }
	bool isColor         () const{ return m_mode == GM_Color || m_mode == GM_RiemannColor; }
	bool isHistogram     () const{ return m_mode == GM_Histogram; }
	bool isGraph         () const
	{
		// returns true if X,Y are completely in the axis box
		switch (m_type)
		{
			case  R_R:
			case R2_R:
			case R3_R: return true;
				
			case  C_C: return m_mode != GM_Image;
				
			case  R_R2:
			case S1_R2:
			case R2_R2:
			case  R_R3:
			case S1_R3:
			case R2_R3:
			case S2_R3:
			case R3_R3: return false;
		}
		assert(false); throw std::logic_error("type corrupted");
	}
	bool wantUprightView () const;
	Axis::Type axisType  () const;
	int inRangeDimension() const; // number of the axis->in_range that are used (zero for S1->X parametrics)
	bool usesShading() const{ return !isLine() && !isVectorField() && !isColor(); }
	bool usesLineColor() const{ return isLine() || isVectorField() || !isColor() && (options.shading_mode == Shading_Points || options.shading_mode == Shading_Wireframe); }
	bool hasFill() const
	{
		return !isLine() && !isVectorField() && !isColor() &&
		(options.shading_mode != Shading_Wireframe &&
		 options.shading_mode != Shading_Points);
	}
	bool hasNormals() const
	{
		return !isLine() && !isVectorField() && !isColor() &&
		(options.shading_mode != Shading_Wireframe &&
		 options.shading_mode != Shading_Points &&
		 options.shading_mode != Shading_Hiddenline);
	}

	std::vector<const Variable*> plotvars() const{ return vars; }
	
	Evaluator *evaluator() const; // current parameter values will already be set
	Expression *expression() const;
	std::set<Parameter*> used_parameters() const;
	bool uses_parameter(const Parameter &p) const;
	bool uses_object(const std::string &name) const;
	
	GraphOptions options;
	
	GL_Graph *gl_graph() const;
	bool needs_update() const{ return m_need_update; }
	void update(ChangeType t);
	void clear_update() const{ m_need_update = false; } // only Plot should call this
	Plot &plot;
	
	std::string description_line() const;
	
#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "Graph " << description_line(); }
#endif

private:
	std::string m_f1, m_f2, m_f3;
	GraphType   m_type;
	GraphMode   m_mode;
	GraphCoords m_coords;
	
	mutable Namespace   ins;
	mutable Expression *ex;
	mutable std::vector<const Variable*> vars;
	
	mutable GL_Graph *gl;
	mutable int m_gl_class; // used inside gl_graph() only
	mutable bool m_need_update; // gl needs recomputing
	
	void invalidate(bool for_init = false);
	void fix_mode();   // set to a valid (for current type) mode
	void fix_coords(); // set to a valid (for current type) coordinate-system
};

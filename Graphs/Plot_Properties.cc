#include "Plot.h"
#include "OpenGL/GL_Color.h"
#include "OpenGL/GL_Font.h"
#include "../Utility/System.h"
#include "../Engine/Namespace/RootNamespace.h"
#include <cassert>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <cassert>

static inline std::string to_string(AntialiasMode m)
{
	switch (m)
	{
		case AA_Off:    return "off";
		case AA_Lines:  return "lines";
		case AA_4x:     return "4x";
		case AA_8x:     return "8x";
		case AA_4x_Acc: return "4a";
		case AA_8x_Acc: return "8a";
	}
	assert(false);
	throw std::logic_error("corruption");
}
static inline AntialiasMode parse_aa(const std::string &s)
{
	if (s == "off" || s == "0") return AA_Off;
	if (s == "lines" || s == "l" || s == "1") return AA_Lines;
	if (s == "4x" || s == "4") return AA_4x;
	if (s == "8x" || s == "8") return AA_8x;
	if (s == "4a") return AA_4x_Acc;
	if (s == "8a") return AA_8x_Acc;
	throw error("Not a valid aa mode", s);
}

double parse_double(const std::string &s, const Namespace &ns)
{
	cnum v = evaluate(s, ns);
	if (!defined(v) || !is_real(v)) throw error("Not a real number", s);
	return v.real();
}
#define VALUES(...) values=[]{ const char *v[]={__VA_ARGS__}; return std::vector<std::string>(v,v+sizeof(v)/sizeof(v[0]));}
void Plot::init_properties()
{
	//-----------------------------------------------------------------------------------
	// PlotOptions
	//-----------------------------------------------------------------------------------

	Property &fog = props["fog"];
	fog.desc = "fog density";
	fog.vis  = [this]{ return axis.type() != Axis::Rect; };
	fog.get  = [this]{ return format("%g", options.fog); };
	fog.set  = [this](const std::string &s){ options.fog = parse_double(s, ns); };

	Property &aa = props["aa"];
	aa.desc = "antialiasing mode";
	aa.get  = [this]{ return to_string(options.aa_mode); };
	aa.set  = [this](const std::string &v){ options.aa_mode = parse_aa(v); };
	aa.VALUES("off", "lines", "4x", "8x", "4a", "8a");
		
	Property &ccp = props["ccp"];
	ccp.desc = "custom clipping plane";
	ccp.vis  = [this]{ return axis.type() != Axis::Rect; };
	ccp.get  = [this]()->std::string{ return 
			!options.clip.on() ? "off" :
			options.clip.locked() ? "lock" : "on"; };
	ccp.set  = [this](const std::string &s)
	{
		GL_ClippingPlane &p = options.clip;
		if (s == "off" || s == "0") p.on(false);
		else if (s == "on" || s == "1"){ p.on(true); p.locked(false); }
		else if (s == "lock" || s == "locked" || s == "l"){ p.on(true); p.locked(true); }
		else throw error("Not a valid clip state", s);
	};
	ccp.VALUES("off", "on", "locked");
	
	Property &ccd = props["ccd"];
	ccd.desc = "custom clipping plane distance";
	ccd.vis  = ccp.vis;
	ccd.get  = [this]()->std::string{ return format("%g", -options.clip.distance()); };
	ccd.set  = [this](const std::string &s)
	{
		GL_ClippingPlane &p = options.clip;
		double d = parse_double(s, ns);
		if (d < -2.0) d = -2.0; else if (d > 2.0) d = 2.0;
		p.distance((float)-d);
	};

	//-----------------------------------------------------------------------------------
	// Axis
	//-----------------------------------------------------------------------------------

	Property &ac = props["ac"];
	ac.desc = "axis color";
	ac.vis  = [this]{ return !axis.options.hidden; };
	ac.get  = [this]{ return axis.options.axis_color.to_string(); };
	ac.set  = [this](const std::string &s){ axis.options.axis_color = s; };
	ac.type = PT_Color;

	Property &bg = props["bg"];
	bg.desc = "background color";
	bg.get  = [this]{ return axis.options.background_color.to_string(); };
	bg.set  = [this](const std::string &s){ axis.options.background_color = s; };
	bg.type = PT_Color;

	Property &af = props["af"];
	af.desc = "axis font";
	af.vis  = [this]{ return !axis.options.hidden; };
	af.get  = [this]{ return axis.options.label_font.to_string(); };
	af.set  = [this](const std::string &s){ axis.options.label_font = s; };
	af.type = PT_Font;

	Property &ax = props["axis"];
	ax.desc = "axis state";
	ax.vis  = [this]{ return axis.type() != Axis::Invalid; };
	ax.get  = [this]()->std::string{ return 
			axis.options.hidden ? "off" : 
			axis.type() != Axis::Rect ? "on" :
			axis.options.axis_grid == AxisOptions::AG_Off ? "line" :
			axis.options.axis_grid == AxisOptions::AG_Cartesian ? "grid" :
			axis.options.axis_grid == AxisOptions::AG_Polar ? "polar" :
			"corrupt";};
	ax.set  = [this](const std::string &s)
	{
		if (s == "off" || s == "0"){ axis.options.hidden =  true; return; }
		if (s == "on"  || s == "1"){ axis.options.hidden = false; return; }
		if (axis.type() != Axis::Rect) throw error("Not a valid (3d) axis state", s);
		if (s == "plain" || s == "line" || s == "l") axis.options.axis_grid = AxisOptions::AG_Off;
		else if (s == "grid"  || s == "g") axis.options.axis_grid = AxisOptions::AG_Cartesian;
		else if (s == "polar" || s == "p") axis.options.axis_grid = AxisOptions::AG_Polar;
		else throw error("Not a valid axis state", s);
		axis.options.hidden = false;
	};
	ax.VALUES("off", "on", "line", "grid", "polar");

	// TODO: axis.options.label_font, axis.options.light
	// TODO: axis.ranges, centers

	//-----------------------------------------------------------------------------------
	// Camera
	//-----------------------------------------------------------------------------------
}

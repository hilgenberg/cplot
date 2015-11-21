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

void Plot::init_properties()
{
	//-----------------------------------------------------------------------------------
	// PlotOptions
	//-----------------------------------------------------------------------------------

	Property &fog = props["fog"];
	fog.desc = "fog density";
	fog.vis  = [this]{ return axis.type() != Axis::Rect; };
	fog.get  = [this]{ return format_percentage(options.fog); };
	fog.set  = [this](const std::string &s){ options.fog = parse_percentage(s); };

	Property &aa = props["aa"];
	aa.desc = "antialiasing mode";
	aa.get  = [this]{ return to_string(options.aa_mode); };
	aa.set  = [this](const std::string &v){ options.aa_mode = parse_aa(v); };
	aa.VALUES("off", "lines", "4x", "8x", "4a", "8a");
		
	Property &ccp = props["cc"];
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
	ccd.get  = [this]()->std::string{ return format_double(-options.clip.distance()); };
	ccd.set  = [this](const std::string &s)
	{
		double d = parse_double(s);
		if (d < -2.0) d = -2.0; else if (d > 2.0) d = 2.0;
		options.clip.distance((float)-d);
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

	// TODO: axis.options.light

	Property &rx = props["rx"], &ry = props["ry"], &rz = props["rz"], &irx = props["irx"], &iry = props["iry"];
	rx.desc  = "axis x range";
	ry.desc  = "axis y range";
	rz.desc  = "axis z range";
	irx.desc = "parametric x range";
	iry.desc = "parametric y range";
	rx.vis  = ry.vis = [this]{ return axis.type() == Axis::Rect || axis.type() == Axis::Box; };
	rz.vis  = [this]{ return axis.type() == Axis::Box; };
	irx.vis = [this]{ return used_inrange() > 0; };
	iry.vis = [this]{ return used_inrange() > 1; };
	rx.get  = [this]()->std::string{ return format_range(axis.min(0), axis.max(0), true); };
	ry.get  = [this]()->std::string{ return format_range(axis.min(1), axis.max(1), true); };
	rz.get  = [this]()->std::string{ return format_range(axis.min(2), axis.max(2), true); };
	irx.get = [this]()->std::string{ return format_range(axis.in_min(0), axis.in_max(0), true); };
	iry.get = [this]()->std::string{ return format_range(axis.in_min(1), axis.in_max(1), true); };
	rx.set  = [this](const std::string &s){ double c = axis.center(0), r = axis.range(0); parse_range(s,c,r); axis.center(0,c); axis.range(0,r); update(CH_AXIS_RANGE); };
	ry.set  = [this](const std::string &s){ double c = axis.center(1), r = axis.range(1); parse_range(s,c,r); axis.center(1,c); axis.range(1,r); update(CH_AXIS_RANGE); };
	rz.set  = [this](const std::string &s){ double c = axis.center(2), r = axis.range(2); parse_range(s,c,r); axis.center(2,c); axis.range(2,r); update(CH_AXIS_RANGE); };
	irx.set = [this](const std::string &s){ double c = axis.in_center(0), r = axis.in_range(0); parse_range(s,c,r); axis.in_center(0,c); axis.in_range(0,r); update(CH_IN_RANGE); };
	iry.set = [this](const std::string &s){ double c = axis.in_center(1), r = axis.in_range(1); parse_range(s,c,r); axis.in_center(1,c); axis.in_range(1,r); update(CH_IN_RANGE); };

	//-----------------------------------------------------------------------------------
	// Camera
	//-----------------------------------------------------------------------------------

	Property &zoom = props["zoom"];
	zoom.desc = "camera zoom";
	zoom.vis  = [this]{ return axis.type() != Axis::Rect; };
	zoom.get  = [this]()->std::string{ return format_double(1.0/camera.zoom()); };
	zoom.set  = [this](const std::string &s){ camera.set_zoom(1.0/parse_double(s)); };

	Property &phi = props["cam.phi"];
	phi.desc = "camera rotation in degrees";
	phi.vis  = zoom.vis;
	phi.get  = [this]()->std::string{ return format("%.2g", camera.phi()); };
	phi.set  = [this](const std::string &s){ camera.set_phi(parse_double(s)); };

	Property &psi = props["cam.psi"];
	psi.desc = "camera elevation in degrees";
	psi.vis  = zoom.vis;
	psi.get  = [this]()->std::string{ return format("%.2g", camera.psi()); };
	psi.set  = [this](const std::string &s){ camera.set_psi(parse_double(s)); };

	Property &theta = props["cam.theta"];
	theta.desc = "camera roll in degrees";
	theta.vis  = zoom.vis;
	theta.get  = [this]()->std::string{ return format("%.2g", camera.theta()); };
	theta.set  = [this](const std::string &s){ camera.set_theta(parse_double(s)); };
}

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

void parse_range(const std::string &s, const Namespace &ns, double &c0, double &r0)
{
	// input range is [c0-r0, c0+r0]
	// syntax: [x0,x1], (x0,x1), [x0;x1], (x0;x1)
	//         xm+-r, r, xm+-

	size_t n = s.length(); if (!n) throw std::runtime_error("Invalid range");

	// intervals
	if (s[0] == '(' && s[n-1] == ')' || s[0] == '[' && s[n-1] == ']')
	{
		int pl = 0, nc = 0, c;
		for (size_t i = 1; i+1 < n; ++i)
		{
			switch (s[i])
			{
				case '(': ++pl; break;
				case ')': --pl; break;
				case ',':
				case ';': if (!pl){ ++nc; c = i; } break;
			}
		}
		if (nc == 1)
		{
			double a = parse_double(s.substr(1, c-1), ns);
			double b = parse_double(s.substr(c+1, n-c-2), ns);
			c0 = (a+b)*0.5;
			r0 = fabs(b-a)*0.5;
			return;
		}
	}

	// m +- r
	int pl = 0, pm = -1;
	for (size_t i = 0; i < n; ++i)
	{
		switch (s[i])
		{
			case '(': ++pl; break;
			case ')': --pl; break;
			case '+': if (!pl && i+1 < n && s[i+1] == '-') pm = i; break;
		}
	}
	if (pm == 0)
	{
		r0 = fabs(parse_double(s.substr(2), ns));
	}
	else if (pm > 0)
	{
		// 1+-2
		double a = parse_double(s.substr(0, pm), ns);
		double b = parse_double(s.substr(pm+2), ns);
		c0 = a;
		r0 = fabs(b);
	}
	else
	{
		c0 = parse_double(s, ns);
	}
}

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
	rx.get  = [this]()->std::string{ return format("[%g;%g]", axis.min(0), axis.max(0)); };
	ry.get  = [this]()->std::string{ return format("[%g;%g]", axis.min(1), axis.max(1)); };
	rz.get  = [this]()->std::string{ return format("[%g;%g]", axis.min(2), axis.max(2)); };
	irx.get = [this]()->std::string{ return format("[%g;%g]", axis.in_min(0), axis.in_max(0)); };
	iry.get = [this]()->std::string{ return format("[%g;%g]", axis.in_min(1), axis.in_max(1)); };
	rx.set  = [this](const std::string &s){ double c = axis.center(0), r = axis.range(0); parse_range(s,ns,c,r); axis.center(0,c); axis.range(0,r); update(CH_AXIS_RANGE); };
	ry.set  = [this](const std::string &s){ double c = axis.center(1), r = axis.range(1); parse_range(s,ns,c,r); axis.center(1,c); axis.range(1,r); update(CH_AXIS_RANGE); };
	rz.set  = [this](const std::string &s){ double c = axis.center(2), r = axis.range(2); parse_range(s,ns,c,r); axis.center(2,c); axis.range(2,r); update(CH_AXIS_RANGE); };
	irx.set = [this](const std::string &s){ double c = axis.in_center(0), r = axis.in_range(0); parse_range(s,ns,c,r); axis.in_center(0,c); axis.in_range(0,r); update(CH_IN_RANGE); };
	iry.set = [this](const std::string &s){ double c = axis.in_center(1), r = axis.in_range(1); parse_range(s,ns,c,r); axis.in_center(1,c); axis.in_range(1,r); update(CH_IN_RANGE); };

	//-----------------------------------------------------------------------------------
	// Camera
	//-----------------------------------------------------------------------------------

	Property &zoom = props["zoom"];
	zoom.desc = "camera zoom";
	zoom.vis  = [this]{ return axis.type() != Axis::Rect; };
	zoom.get  = [this]()->std::string{ return format("%g", 1.0/camera.zoom()); };
	zoom.set  = [this](const std::string &s){ camera.set_zoom(1.0/parse_double(s, ns)); };

	Property &phi = props["cam.phi"];
	phi.desc = "camera rotation in degrees";
	phi.vis  = zoom.vis;
	phi.get  = [this]()->std::string{ return format("%.2g", camera.phi()); };
	phi.set  = [this](const std::string &s){ camera.set_phi(parse_double(s, ns)); };

	Property &psi = props["cam.psi"];
	psi.desc = "camera elevation in degrees";
	psi.vis  = zoom.vis;
	psi.get  = [this]()->std::string{ return format("%.2g", camera.psi()); };
	psi.set  = [this](const std::string &s){ camera.set_psi(parse_double(s, ns)); };

	Property &theta = props["cam.theta"];
	theta.desc = "camera roll in degrees";
	theta.vis  = zoom.vis;
	theta.get  = [this]()->std::string{ return format("%.2g", camera.theta()); };
	theta.set  = [this](const std::string &s){ camera.set_theta(parse_double(s, ns)); };
}

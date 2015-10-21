#include "Info.h"
#include "../Graph.h"
#include "../../Engine/Namespace/Variable.h"

DI_Calc::DI_Calc(Graph &graph) : embed_XZ(false)
{
	e0 = graph.evaluator(); if (!e0) return;
	dim = e0->image_dimension();
	polar = spherical = false;
	switch (graph.coords())
	{
		case GC_Polar:
		case GC_Cylindrical: polar = true; break;
		case GC_Spherical:   spherical = true; break;
		default: break;
	}
	
	std::vector<const Variable *> pvars = graph.plotvars();
	const Variable *x = (pvars.size() > 0 ? pvars[0] : NULL);
	const Variable *y = (pvars.size() > 1 ? pvars[1] : NULL);
	const Variable *z = (pvars.size() > 2 ? pvars[2] : NULL);
	xi = (x ? e0->var_index(x) : -1); // = z for complex functions
	yi = (y ? e0->var_index(y) : -1);
	zi = (z ? e0->var_index(z) : -1);

	complex      = (graph.type() == C_C);
	implicit     = (graph.type() == R3_R);
	vector_field = graph.isVectorField();
	projection   = graph.mode();
}

DI_Axis::DI_Axis(Graph &graph, bool is_graph, bool S1) : S1(S1)
{
	const Axis &axis = graph.plot.axis;
	const Camera &camera = graph.plot.camera;
	pixel = camera.pixel_size(axis);
	is2D  = (axis.type() == Axis::Rect);
	bool isRiemann = (axis.type() == Axis::Sphere);

	clipping = (is2D || (!isRiemann && graph.clipping() && !graph.plot.options.clip.on()));

	for (int i = 0; i < 3; ++i)
	{
		center[i] = axis.center(i);
		range [i] = axis.range (i);
	}
	// avoid undefined values and display glitches
	center[0] += M_PI*1e-6*range[0];
	center[1] -= M_E*1e-7*range[1];
	center[2] += 1e-8*range[2];
	
	for (int i = 0; i < 3; ++i)
	{
		min[i] = center[i] - range[i];
		max[i] = center[i] + range[i];
	}
	
	if (is_graph)
	{
		for (int i = 0; i < 3; ++i)
		{
			in_center[i] = center[i];
			in_range [i] = range [i];
		}
	}
	else if (S1)
	{
		for (int i = 0; i < 3; ++i)
		{
			in_center[i] = M_PI;
			in_range [i] = M_PI;
		}
	}
	else
	{
		for (int i = 0; i < 2; ++i)
		{
			in_center[i] = axis.in_center(i);
			in_range [i] = axis.in_range (i);
		}
		in_center[2] = 0.0;
		in_range [2] = 0.0;
	}

	for (int i = 0; i < 3; ++i) in_center[i] += M_PI*1e-12*in_range[i]; // avoid undefined values

	for (int i = 0; i < 3; ++i)
	{
		in_min[i] = in_center[i] - in_range[i];
		in_max[i] = in_center[i] + in_range[i];
	}
	
	yh = range[1] / range[0];
	zh = range[2] / range[0];
}

void DI_Grid::Grid::init(double x00_, double x11_, double n_visible_gridlines, int min_gridlines, bool tight)
{
	single_cell = (n_visible_gridlines <= 1.0);
	border_ok   = (min_gridlines > 0);

	x00 = x00_;
	x11 = x11_;
	double xr = x11 - x00; // can be negative, which is ok
	if (tight || n_visible_gridlines < 2.0)
	{
		x0 = x00; // = 0
		x1 = x11; // = 2π
		n = std::max((int)floor(n_visible_gridlines), 2);
		if (n < min_gridlines)
		{
			// we want #gridlines = n + subdiv*(n-1) >= min_gridlines, so
			//  subdiv =  ceil( (min_gridlines - n) / (n-1) )
			//         = floor( (min_gridlines - n + (n-1 - 1)) / (n-1) )
			//         = floor( (min_gridlines - 2) / (n-1) )
			int subdiv = (min_gridlines-2) / (n-1);
			n += (n-1)*subdiv;
			assert(n >= min_gridlines);
			dvis = subdiv+1;
		}
		else
		{
			dvis = 1; // no subdivision needed
		}
		vis0 = dvis;
		dx = xr / (n - 1);
		b0 = b1 = false;
		N = n;
	}
	else
	{
		// one visible gridline will always be in the middle of the range
		double xm = 0.5*x00 + 0.5*x11;
		dx = xr / (n_visible_gridlines - 1.0);
		n = 1 /* at xm */ + 2 * (int)floor(0.5*xr / dx); // note xr/dx >= 0
		if (n < min_gridlines)
		{
			// we want n = 1 + 2floor(0.5*xr/(dx/subdiv)) >= min_gridlines, so
			// floor(0.5*xr/(dx/subdiv)) >= (min_gridlines-1)/2
			// floor(xr subdiv / 2dx) >= (min_gridlines-1)/2
			//   note [floor(x) >= y iff x >= ceil(y)]
			//   and  [ceil(n/m) = floor((n+m-1)/m)]
			// xr subdiv / 2dx >= ceil((min_gridlines-1)/2)
			// subdiv >= ceil((min_gridlines-1)/2) * 2dx / xr
			//         = floor(min_gridlines/2) * 2dx / xr
			//         = floor(min_gridlines/2) * 2(xr / (n_visible_gridlines - 1.0)) / xr
			//         = floor(min_gridlines/2) * 2 / (n_visible_gridlines - 1.0)
			
			int subdiv = (int)ceil( ((min_gridlines/2) * 2) / (n_visible_gridlines - 1.0) );
			assert(subdiv > 1);
			dx /= subdiv;
			n = 1 /* at xm */ + 2 * (int)floor(0.5*xr / dx);
			assert(n >= min_gridlines - 1); // allow 1 off for rounding errors
			dvis = subdiv;
			vis0 = dvis - (n-1)/2 % dvis;
		}
		else
		{
			dvis = 1;
			vis0 = dvis;
		}
		x0 = xm - dx * ((n-1)/2);
		x1 = xm + dx * ((n-1)/2);
		b0 = (min_gridlines > 0 && fabs(x0 - x00) > 0.1*fabs(dx));
		b1 = (min_gridlines > 0 && fabs(x11 - x1) > 0.1*fabs(dx));
		
		N = n; if (b0) ++N; if (b1) ++N;
	}
}

void DI_Grid::Grid::init(const DI_Grid::Grid &g, double x00_, double x11_, bool tight)
{
	single_cell = g.single_cell;
	border_ok   = g.border_ok;
	x00 = x00_;
	x11 = x11_;
	double xr = x11 - x00; // can be negative, which is ok

	if (tight || single_cell)
	{
		x0 = x00; // = 0
		x1 = x11; // = 2π
		n = g.n;
		dx = xr / (n-1);
		dvis = g.dvis;
		vis0 = dvis;
		b0 = b1 = false;
		N = n;
	}
	else
	{
		dx = g.dx;
		double xm = 0.5*x00 + 0.5*x11;
		n = 1 /* at xm */ + 2 * (int)floor(0.5*xr / dx);
		dvis = g.dvis;
		vis0 = dvis - (n-1)/2 % dvis;
		x0 = xm - dx * ((n-1)/2);
		x1 = xm + dx * ((n-1)/2);
		b0 = (g.border_ok && fabs(x0 - x00) > 0.1*fabs(dx));
		b1 = (g.border_ok && fabs(x11 - x1) > 0.1*fabs(dx));
		
		N = n; if (b0) ++N; if (b1) ++N;
	}
	
	if (n < 2)
	{
		n = N = 2;
		dx = xr;
		x0 = x00;
		x1 = x11;
		dvis = 4;
		vis0 = 1;
		b0 = b1 = false;
	}
}
void DI_Grid::Grid::init(double x)
{
	x00 = x11 = x0 = x1 = x;
	n = N = 1;
	dx = 0.0;
	vis0 = 1;
	dvis = 1;
	b0 = b1 = false;
}

DI_Grid::DI_Grid(DI_Axis &a, double density, int min_gridlines, bool is_3d)
{
	if (a.S1)
	{
		x.init(a.in_min[0], a.in_max[0], density, min_gridlines, true);
		y.init(a.in_min[1], a.in_max[1], density, min_gridlines, true);
		z.init(0.0);
	}
	else
	{
		double xr = 2.0 * fabs(a.in_range[0]);
		double yr = 2.0 * fabs(a.in_range[1]);
		double zr = 2.0 * fabs(a.in_range[2]);

		if (xr >= yr && (!is_3d || xr >= zr))
		{
			x.init(a.in_min[0], a.in_max[0], density, min_gridlines, false);
			y.init(x, a.in_min[1], a.in_max[1], false);
			is_3d ? z.init(x, a.in_min[2], a.in_max[2], false) : z.init(a.in_center[2]);
		}
		else if (!is_3d || yr >= zr)
		{
			y.init(a.in_min[1], a.in_max[1], density, min_gridlines, false);
			x.init(y, a.in_min[0], a.in_max[0], false);
			is_3d ? z.init(y, a.in_min[2], a.in_max[2], false) : z.init(a.in_center[2]);
		}
		else
		{
			z.init(a.in_min[2], a.in_max[2], density, min_gridlines, false);
			x.init(z, a.in_min[0], a.in_max[0], false);
			y.init(z, a.in_min[1], a.in_max[1], false);
		}
	}
}

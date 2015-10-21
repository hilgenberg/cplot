#pragma once
#include "../../Engine/Parser/Evaluator.h"
#include "../../Utility/MemoryPool.h"
#include "../Geometry/Axis.h"
#include "../Geometry/Camera.h"
#include "../Plot.h"

//----------------------------------------------------------------------------------------------------------------------
// Structs with information and storage needed for calculation
//----------------------------------------------------------------------------------------------------------------------

struct DI_Axis
{
	DI_Axis(Graph &graph, bool is_graph, bool is_S1);
	
	double center[3], range[3], min[3], max[3]; // for 2D-axis <all>[2] = 0, for Riemann: [-1,1] in every dimension
	double in_center[3], in_range[3], in_min[3], in_max[3]; // for graphs this is copied from min/max
	double yh, zh;  // y- and z-range in axis coordinates
	double pixel;
	bool   S1;         // in_range is S1 or S1^2
	bool   clipping;   // to min/max range
	bool   is2D;
	
	inline bool visible(const P2f &p) const
	{
		return fabsf(p.x) <= 1.0f && fabsf(p.y) <= yh;
	}
	
	#define COORD_MAX 100.0
	inline void map(const P3d &p, P3f &q) const
	{
		double rxi = 1.0 / range[0];
		double x = (p.x - center[0]) * rxi;
		double y = (p.y - center[1]) * rxi;
		double z = (p.z - center[2]) * rxi;
		
		q.x = fabs(x) < COORD_MAX ? (float)x : x < 0 ? -COORD_MAX : COORD_MAX;
		q.y = fabs(y) < COORD_MAX ? (float)y : y < 0 ? -COORD_MAX : COORD_MAX;
		q.z = fabs(z) < COORD_MAX ? (float)z : z < 0 ? -COORD_MAX : COORD_MAX;
	}
	inline void map_vector(const P3d &p, P3f &q) const
	{
		double rxi = 1.0 / range[0];
		double x = p.x * rxi;
		double y = p.y * rxi;
		double z = p.z * rxi;
		
		q.x = fabs(x) < COORD_MAX ? (float)x : x < 0 ? -COORD_MAX : COORD_MAX;
		q.y = fabs(y) < COORD_MAX ? (float)y : y < 0 ? -COORD_MAX : COORD_MAX;
		q.z = fabs(z) < COORD_MAX ? (float)z : z < 0 ? -COORD_MAX : COORD_MAX;
	}
	inline float map_size(double s) const
	{
		return (float)(s / range[0]);
	}
	#undef COORD_MAX
	
	inline void map_texture(double u, double v, P2f &q) const
	{
		q.x = (float)(0.5 * (u - in_center[0]) / in_range[0] + 0.5);
		q.y = (float)(0.5 * (in_center[1] - v) / in_range[1] + 0.5);
	}
	inline double elevation(double u, double v) const
	{
		return center[2] + 0.005 * range[2] * ( (u - in_min[0]) / in_range[0] + (v - in_min[1]) / in_range[1]);
	}
};

struct DI_Subdivision
{
	DI_Subdivision()
	#ifdef DEBUG
	: ssb_area(0), ssb_kink(0), ssb_len(0), ssb_plen(0), ssb_eodef(0)
	#endif
	{}

	size_t  max_faces;
	double  max_kink;
	double  max_lenq, min_lenq;
	double  disco_limit;
	bool    detect_discontinuities;
	
	#ifdef DEBUG
	mutable size_t ssb_area, ssb_kink, ssb_len, ssb_plen, ssb_eodef; // subdivision tuning info
	#endif
};

class Graph;
struct DI_Calc
{
	DI_Calc(Graph &graph);
	
	Evaluator *e0;         // use bound context instead!
	int     xi, yi, zi;    // variable indexes
	
	int  dim;              // output dimensions
	bool polar, spherical; // cylindrical is set as polar
	bool complex;
	bool vector_field;
	bool implicit;
	bool embed_XZ;         // for 2d-graphs: (x,y) -> (x,0,y) instead of (x,y,0), must never be set for 2D axis
	bool vertex_normals;   // calculate vertex normals?
	bool face_normals;     // calculate face normals for flatshading?
	bool texture;          // calculate texture coordinates?
	bool do_grid;
	GraphMode projection;
};


//----------------------------------------------------------------------------------------------------------------------
// Grid: split an interval into a grid (a regular inside grid and optionally two additional border lines)
// - [x00, x11] is the full interval
// - [x0, x1] is from the first regular gridline to the last
// - if b0 is true, the first gridline is at x00, the second at x0, the third at x0+dx, ...
// - if b0 is false, the first gridline is at x0, the second at x0+dx, ...
// - if b1 is true, the last gridline is at x11
// - the lines at i == vis0 + k*dvis are the visible gridlines
// - border lines are never visible
//----------------------------------------------------------------------------------------------------------------------

struct DI_Grid
{
	struct Grid
	{
		friend struct DI_Grid;
		
		Grid() : n(-1), N(-1){} // dummy grid for point and line graphs
		
		inline int nlines() const{ return N; }
		
		#define CHK(i) assert(i >= 0 && i < nlines())
		inline double operator[](int i) const{ CHK(i); return (b0 && --i < 0) ? x00 : i < n ? x0 + i*dx : x11; }
		inline bool visible(int i) const{ CHK(i); return (!b0 || --i >= 0) && ((i + vis0) % dvis == 0) && (!b1 || i < n); }
		#undef CHK
		
		double     delta() const{ return dx; }
		double vis_delta() const{ return dvis*dx; }
		double first_vis() const{ return operator[](dvis-vis0+(b0 ? 1 : 0)); }

	private:
		// [x0, x1] is the full interval, which is divided into n_visible_gridlines-1 parts and
		// further subdivided (by adding invisible gridlines) so there are at least min_gridlines.
		// If tight is true, it guarantees [x00, x11]=[x0, x1] (so there are no borders (for S1))
		void init(double x00, double x11, double n_visible_gridlines, int min_gridlines, bool tight);
		void init(const Grid &g, double x00, double x11, bool tight);
		void init(double x);
		
		double x0,  x1;  // first grid line, grid.x0 >= x0, last grid line
		double x00, x11; // DI_Axis::x0, etc for the border lines
		int    n;        // not including border lines
		int    N;        // including border lines
		
		int    dvis;     // every dvis'th grid line is visible
		double dx;       // gridlines at x0, x0+dx, x0+2dx, ...
		int    vis0;     // dvis minus index of first visible (as in edge flag) grid lines, not counting border lines
		bool   b0, b1;   // first gridline at x00? And b1 for the other side

		bool   single_cell; // for copy-init
		bool   border_ok;   // same here
	};
	
	
	// density: 0..100, number of visible grid lines minus 2
	// min_gridlines is for the larger of the x/y ranges only
	DI_Grid(DI_Axis &a, double density, int min_gridlines, bool is_3d); // min_gridlines must be 0 for point graphs
	DI_Grid(){} // dummy grid for point, line and image graphs

	Grid x, y, z;
};

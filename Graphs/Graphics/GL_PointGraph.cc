#include "GL_PointGraph.h"
#include "../Geometry/Vector.h"
#include "../../Utility/MemoryPool.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "../Geometry/RecursiveGrid.h"
#include "../OpenGL/GL_Util.h"

#include <vector>
#include <algorithm>

#define VF_SUB 8

//----------------------------------------------------------------------------------------------------------------------
// GraphPoint - element of a point graph or vector field
//----------------------------------------------------------------------------------------------------------------------

struct GraphPoint
{
	POOL_ITEM_BASE(GraphPoint);
	
	P3d  x;
	P3f  p, v;
	bool exists; // has real values?
	
	GraphPoint(){ }
	GraphPoint(const GraphPoint &) = default;
	
	inline void calc(ThreadInfo &ti, bool same_u = false, bool same_v = false, bool same_w = false)
	{
		const DI_Calc &ic = ti.ic;
		if (ic.vector_field)
		{
			switch (ic.dim)
			{
				case 2:
				{
					ti.eval_vector(x.x, x.y, v, exists, same_u, same_v);
					if (exists) ti.ia.map(x, p);
					break;
				}
				case 3:
				{
					ti.eval_vector(x.x, x.y, x.z, v, exists, same_u, same_v, same_w);
					if (exists) ti.ia.map(x, p);
					break;
				}
				default:
					assert(false);
					break;
			}
		}
		else
		{
			ti.eval(x.x, x.y, p, exists, same_u, same_v);
		}
	}
};

//----------------------------------------------------------------------------------------------------------------------
// Grid building
//----------------------------------------------------------------------------------------------------------------------

static void gridBuilder(ThreadInfo &ti, int i1, int i2, GraphPoint *vs, size_t &nv_exist)
{
	const DI_Grid &ig = ti.ig;
	const DI_Calc &ic = ti.ic;
	
	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	int nz = ig.z.nlines();

	nv_exist = 0;

	if (ic.implicit)
	{
		for (int k = 0; k < nz; ++k)
		{
			double zk = ig.z[k];
			for (int i = i1; i < i2; ++i)
			{
				double yi = ig.y[i];
				for (int j = 0; j < nx; ++j)
				{
					double xj = ig.x[j];
					GraphPoint &v  = *new(vs + nx*ny*k + nx*i + j) GraphPoint;
					double f = ti.eval(xj, yi, zk);
					v.exists = (defined(f) && f <= 0.0);
					if (v.exists)
					{
						++nv_exist;
						v.x.set(xj, yi, zk);
						ti.ia.map(v.x, v.p);
					}
					else
					{
						v.x.clear();
						v.p.clear();
					}
				}
			}
		}
	}
	else if (ic.vector_field && ic.dim == 3)
	{
		for (int k = 0; k < nz; ++k)
		{
			double zk = ig.z[k];
			for (int i = i1; i < i2; ++i)
			{
				double yi = ig.y[i];
				for (int j = 0; j < nx; ++j)
				{
					double xj = ig.x[j];
					GraphPoint &v  = *new(vs + nx*ny*k + nx*i + j) GraphPoint;
					v.x.set(xj, yi, zk);
					v.calc(ti, false, j>0);
					if (v.exists) ++nv_exist;
				}
			}
		}
	}
	else
	{
		for (int i = i1; i < i2; ++i)
		{
			double yi = ig.y[i];
			for (int j = 0; j < nx; ++j)
			{
				double xj = ig.x[j];
				GraphPoint &v  = *new(vs + nx*i + j) GraphPoint;
				v.x.set(xj, yi, 0.0);
				v.calc(ti, false, j>0);
				if (v.exists) ++nv_exist;
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Subdivision
//----------------------------------------------------------------------------------------------------------------------

struct StreamInfo3D
{
	StreamInfo3D(const DI_Grid &ig, ThreadInfo &ti, MemoryPool<GraphPoint> &pool, RecursiveGrid_3D &grid, float max_len)
	: x00(ig.x[0]), x11(ig.x[ig.x.nlines()-1])
	, y00(ig.y[0]), y11(ig.y[ig.y.nlines()-1])
	, z00(ig.z[0]), z11(ig.z[ig.z.nlines()-1])
	, ti(ti)
	, nnx((ig.x.nlines()-1)*VF_SUB + 1)
	, nny((ig.y.nlines()-1)*VF_SUB + 1)
	, nnz((ig.z.nlines()-1)*VF_SUB + 1)
	, pool(pool)
	, grid(grid)
	{
		scale = std::min((x11-x00)/nnx, std::min((y11-y00)/nny, (z11-z00)/nnz)) * VF_SUB*0.8 / max_len;
	}

	double x00, x11, y00, y11, z00, z11;
	double scale;
	int    nnx, nny, nnz;
	ThreadInfo &ti;
	MemoryPool<GraphPoint> &pool;
	RecursiveGrid_3D &grid;
};
struct StreamInfo2D
{
	StreamInfo2D(const DI_Grid &ig, ThreadInfo &ti, MemoryPool<GraphPoint> &pool, RecursiveGrid_2D &grid, float max_len)
	: x00(ig.x[0]), x11(ig.x[ig.x.nlines()-1])
	, y00(ig.y[0]), y11(ig.y[ig.y.nlines()-1])
	, ti(ti)
	, nnx((ig.x.nlines()-1)*VF_SUB + 1)
	, nny((ig.y.nlines()-1)*VF_SUB + 1)
	, pool(pool)
	, grid(grid)
	{
		scale = std::min((x11-x00)/nnx, (y11-y00)/nny) * VF_SUB*0.8 / max_len;
	}
	
	double x00, x11, y00, y11;
	double scale;
	int    nnx, nny;
	ThreadInfo &ti;
	MemoryPool<GraphPoint> &pool;
	RecursiveGrid_2D &grid;
};

#define SQ 20

static inline void stream(int x, int y, int z, GraphPoint p, const StreamInfo3D &si, size_t &nv)
{
	if (!p.exists || si.grid.get_range(x,y,z,VF_SUB/2)) return;
	//for (int np = 2 + random() % 7; np > 0; --np)
	while (true)
	{
		assert(!si.grid.get(x,y,z));
		si.grid.set(x,y,z);
		
		new(si.pool) GraphPoint(p);
		++nv;

		for (int i = 0; i < SQ; ++i)
		{
			p.x += (P3d)p.v * si.scale / SQ;
			p.calc(si.ti);
			if (!p.exists) return;
		}
		
		x = (int)((p.x.x - si.x00) / (si.x11-si.x00) * (si.nnx-1));
		y = (int)((p.x.y - si.y00) / (si.y11-si.y00) * (si.nny-1));
		z = (int)((p.x.z - si.z00) / (si.z11-si.z00) * (si.nnz-1));
		if (x < 0 || y < 0 || z < 0 || x >= si.nnx || y >= si.nny || z >= si.nnz) break;
		//if (si.grid.get_range(x, y, z, 2)) break;
		//if (si.grid.get_range(x, y, z, 1)) break;
		if (si.grid.get(x, y, z)) break;
	}
}

static inline bool move(int &x0, int &y0, int x1, int y1, RecursiveGrid_2D &grid)
{
	if (!grid.valid(x1, y1))
	{
		grid.set(x0,y0);
		return true;
	}
	
	bool ret = grid.get(x1,y1);
	
	int dx = abs(x1-x0);
	int dy = abs(y1-y0);
	int sx = (x0 < x1 ? 1 : -1);
	int sy = (y0 < y1 ? 1 : -1);
	int err = dx-dy;
			
	while (true)
	{
		grid.set(x0,y0);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2*err;
		if (e2 > -dy)
		{
			err -= dy;
			x0 += sx;

			if (x0 == x1 && y0 == y1){ grid.set(x0,y0); break; }
		}
		if (e2 < dx)
		{
			err += dx;
			y0 += sy;
		}
	}
	
	return ret;
}
static inline void stream(int x, int y, GraphPoint p, const StreamInfo2D &si, size_t &nv)
{
	if (!p.exists || si.grid.get_range(x,y,VF_SUB/4)) return;
	while (true)
	{
		si.grid.set(x,y);
		new(si.pool) GraphPoint(p);
		++nv;
		
		for (int i = 0; i < SQ; ++i)
		{
			p.x += (P3d)p.v * si.scale / SQ;
			p.calc(si.ti);
			if (!p.exists) return;
		}
		
		if (move(x, y,
			 (int)((p.x.x - si.x00) / (si.x11-si.x00) * (si.nnx-1)),
			 (int)((p.x.y - si.y00) / (si.y11-si.y00) * (si.nny-1)),
			 si.grid)) break;
		
		if (x < 0 || y < 0 || x >= si.nnx || y >= si.nny) break;
	}
}

static void connect(ThreadInfo &ti, GraphPoint *vs, size_t &nvertexes, float &max_len,
					std::unique_ptr <P3f[]> &pau, std::unique_ptr <P3f[]> &vau)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) gather info
	//------------------------------------------------------------------------------------------------------------------

	const DI_Grid &ig = ti.ig;
	const DI_Calc &ic = ti.ic;
	
	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	int nz = ig.z.nlines();
	int nnx = (nx-1)*VF_SUB + 1;
	int nny = (ny-1)*VF_SUB + 1;
	int nnz = (nz-1)*VF_SUB + 1;
	assert(ic.vector_field);
		
	//------------------------------------------------------------------------------------------------------------------
	// (2) find scale factor from the grid points we already have
	//------------------------------------------------------------------------------------------------------------------
	
	max_len = 0.0f;
	for (int i = 0; i < nx*ny*nz; ++i)
	{
		if (!vs[i].exists) continue;
		float rq = vs[i].v.absq();
		if (rq > max_len) max_len = rq;
	}
	max_len = sqrtf(max_len);

	//------------------------------------------------------------------------------------------------------------------
	// (3) subdivide
	//------------------------------------------------------------------------------------------------------------------
	
	MemoryPool<GraphPoint> pool;
	nvertexes = 0;

	if (ic.dim == 3)
	{
		RecursiveGrid_3D rg(nnx, nny, nnz, 4);
		StreamInfo3D info(ig, ti, pool, rg, max_len);

		for (int x0 = 0, x1 = nx-1, y0 = 0, y1 = ny-1, z0 = 0, z1 = nz-1;
			 x0 <= x1 && y0 <= y1 && z0 <= z1;
			 ++x0, ++y0, ++z0, --x1, --y1, --z1)
		{
			#define STREAM(x,y,z) stream(x*VF_SUB, y*VF_SUB, z*VF_SUB, vs[nx*ny*z + nx*y + x], info, nvertexes)
			
			for (int y = y0; y <= y1; ++y)
			{
				for (int x = x0; x <= x1; ++x)
				{
					STREAM(x, y, z0);
					if (z1 > z0) STREAM(x, y, z1);
				}
			}

			for (int z = z0+1; z < z1; ++z)
			{
				for (int x = x0; x <= x1; ++x)
				{
					STREAM(x, y0, z);
					if (y1 > y0) STREAM(x, y1, z);
				}
			}

			for (int z = z0+1; z < z1; ++z)
			{
				for (int y = y0+1; y < y1; ++y)
				{
					STREAM(x0, y, z);
					if (x1 > x0) STREAM(x1, y, z);
				}
			}
			
			#undef STREAM
		}
	}
	else
	{
		RecursiveGrid_2D rg(nnx, nny, 4);
		StreamInfo2D info(ig, ti, pool, rg, max_len);
		
		for (int x0 = 0, x1 = nx-1, y0 = 0, y1 = ny-1;
			 x0 <= x1 && y0 <= y1;
			 ++x0, ++y0, --x1, --y1)
		{
			#define STREAM(x,y) stream(x*VF_SUB, y*VF_SUB, vs[nx*y + x], info, nvertexes)
			
			for (int x = x0; x <= x1; ++x)
			{
				STREAM(x, y0);
				if (y1 > y0) STREAM(x, y1);
			}
			
			for (int y = y0+1; y < y1; ++y)
			{
				STREAM(x0, y);
				if (x1 > x0) STREAM(x1, y);
			}
			
			#undef STREAM
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	// (4) transfer
	//------------------------------------------------------------------------------------------------------------------

	pau.reset(new P3f[nvertexes]);
	vau.reset(new P3f[nvertexes]);
	P3f *pa = pau.get();
	P3f *va = vau.get();
	
	max_len = 0.0f;
	for (GraphPoint &gp : pool)
	{
		*pa++ = gp.p;
		*va++ = gp.v;
		float rq = gp.v.absq();
		if (rq > max_len) max_len = rq;
	}
	max_len = sqrtf(max_len);
}

//----------------------------------------------------------------------------------------------------------------------
// drawing helpers
//----------------------------------------------------------------------------------------------------------------------

static void transfer(ThreadInfo &ti, size_t &nvertexes, float &max_len, GraphPoint *vs,
					 std::vector<size_t> &nvs_exist,
					 std::unique_ptr<P3f[]> &pau, std::unique_ptr<P3f[]> &vau)
{
	const DI_Grid &ig = ti.ig;
	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	int nz = ig.z.nlines();
	bool vectorfield = ti.ic.vector_field;
	
	max_len = 0.0f;
	
	nvertexes = 0;
	for (size_t &n : nvs_exist) nvertexes += n;
	#ifndef NDEBUG
	size_t nv = nvertexes;
	#endif
	
	pau.reset(new P3f[nvertexes]);
	vau.reset(vectorfield ? new P3f[nvertexes] : NULL);
	P3f *pa = pau.get();
	P3f *va = vau.get();

	for (int i = 0; i < nx*ny*nz; ++i)
	{
		if (!vs[i].exists) continue;
		assert(nv-- > 0);
		*pa++ = vs[i].p;
		if (vectorfield)
		{
			*va++ = vs[i].v;
			float rq = vs[i].v.absq();
			if (rq > max_len) max_len = rq;
		}
	}
	assert(nv == 0);
	max_len = sqrtf(max_len);
}

//----------------------------------------------------------------------------------------------------------------------
// update and draw
//----------------------------------------------------------------------------------------------------------------------

//static inline double sqr(double x){ return x*x; }

void GL_PointGraph::update(int nthreads, double /*quality*/)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim <= 0 || ic.dim > 3) return;
	
	bool circle, parametric;
	switch (graph.type())
	{
		case  C_C:  circle = false; parametric = (graph.mode()==GM_Image || graph.mode()==GM_Riemann); break;
		case R2_R:  circle = false; parametric = false; break;
		case R2_R3: circle = false; parametric = true;  break;
		case S2_R3: circle = true;  parametric = true;  break;
		case R2_R2: circle = false; parametric = (graph.mode()==GM_Image);  break;
		case R3_R3: circle = false; parametric = false; break;
		case R3_R:  circle = false; parametric = false; break;
		default: return;
	}
	
	DI_Axis ia(graph, !parametric, circle);
	if (ia.pixel <= 0.0) return;
	
	bool threeD = (graph.type()==R3_R3 || graph.type()==R3_R);
	bool vf_subdiv = (graph.isVectorField() && graph.options.vf_mode == VF_Connected);
	double gd = graph.options.grid_density * ((threeD && graph.isVectorField()) ? 0.2 : 1.0);
	if (!graph.isVectorField()) gd *= 1.0 + 9.0*graph.options.quality;
	
	DI_Grid ig(ia, gd, 0, threeD);
	
	gridsize = ia.map_size(std::min(ig.x.delta(), threeD ? std::min(ig.y.delta(), ig.z.delta()) : ig.y.delta()));
	
	int nx = ig.x.nlines(), ny = ig.y.nlines(), nz = ig.z.nlines();

	DI_Subdivision dummy;
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&dummy);
	info.push_back(&ig);

	RawMemoryPool pool;
	Task task(&info);
	
	//------------------------------------------------------------------------------------------------------------------
	// (2) build the grid
	//------------------------------------------------------------------------------------------------------------------
	
	GraphPoint *vs = (GraphPoint*) pool.alloc(sizeof(GraphPoint) * nx*ny*nz);
	
	WorkLayer *layer = new WorkLayer("gridLayer", &task, NULL);
	
	int chunk = (ny+nthreads-1) / nthreads;
	if (chunk < 2) chunk = 2;
	std::vector<size_t> nvs_exist((ny+chunk-1)/chunk);
	for (int i = 0, j = 0; i < ny; i += chunk, ++j)
	{
		int i1 = i + chunk;
		if (i1 > ny) i1 = ny;
		layer->add_unit([=,&nvs_exist,&vs](void *ti)
		{
			gridBuilder(*(ThreadInfo*)ti, i, i1, vs, nvs_exist[j]);
		});
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// (3a) subdivide/connect + allocate and fill the arrays
	// (3b) allocate and fill the arrays
	//------------------------------------------------------------------------------------------------------------------
	
	if (vf_subdiv)
	{
		layer = new WorkLayer("connectionLayer", &task, layer, 1, -1);
		layer->add_unit([&](void *ti)
		{
			connect(*(ThreadInfo*)ti, vs, nvertexes, max_len, pau, vau);
		});
	}
	else
	{
		layer = new WorkLayer("transferLayer", &task, layer, 1, -1);
		layer->add_unit([&](void *ti)
		{
			transfer(*(ThreadInfo*)ti, nvertexes, max_len, vs, nvs_exist, pau, vau);
		});
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// (5) run the entire thing
	//------------------------------------------------------------------------------------------------------------------
	
	task.run(nthreads);
}

Opacity GL_PointGraph::opacity() const
{
	return graph.options.line_color.opaque() ? ANTIALIASED : TRANSPARENT;
}

bool GL_PointGraph::needs_depth_sort() const
{
	return graph.plot.options.aa_mode == AA_Lines || !graph.options.line_color.opaque();
}

void GL_PointGraph::draw(GL_RM &/*rm*/) const
{
	start_drawing();
	
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(graph.options.line_color.opaque() && !glIsEnabled(GL_LINE_SMOOTH));
	graph.options.line_color.set();

	P3f *pa = pau.get();
	if (graph.isVectorField())
	{
		P3f *va = vau.get();
		float tip = float(15.0*graph.plot.pixel_size()/graph.plot.axis.range(0));
		float f;
		bool unit;
		
		switch (graph.options.vf_mode)
		{
			case VF_Unscaled:   f = 1.0f;                     unit = false; break;
			case VF_Normalized: f = gridsize*0.9f  / max_len; unit = false; break;
			case VF_Connected:  f = gridsize*0.75f / max_len; unit = false; break;
			case VF_Unit:       f = gridsize*0.75f;           unit = true;  break;
		}

		glLineWidth(1.0f);

		if (unit)
		{
			P3f v;
			if (graph.type() == R2_R2)
			{
				for (size_t i = 0; i < nvertexes; ++i){ v = va[i]; v.to_unit(); draw_arrow2d(pa[i], v*f, tip); }
			}
			else
			{
				for (size_t i = 0; i < nvertexes; ++i){ v = va[i]; v.to_unit(); draw_arrow3d(pa[i], v*f, tip); }
			}
		}
		else
		{
			if (graph.type() == R2_R2)
			{
				for (size_t i = 0; i < nvertexes; ++i) draw_arrow2d(pa[i], va[i] * f, tip);
			}
			else
			{
				for (size_t i = 0; i < nvertexes; ++i) draw_arrow3d(pa[i], va[i] * f, tip);
			}
		}
	}
	else
	{
		glPointSize((GLfloat)graph.options.line_width);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, pa);
		glDrawArrays(GL_POINTS, 0, (int)nvertexes);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	
	finish_drawing();
}

//----------------------------------------------------------------------------------------------------------------------
// Depth sorting
//----------------------------------------------------------------------------------------------------------------------

struct Iter // for sorting vectors and their basepoints simultaneously
{
	typedef ptrdiff_t          difference_type;
	typedef std::pair<P3f,P3f> value_type; // for temp copies in the STL code
	typedef value_type         Val;
	
	struct Ref
	{
		inline Ref(const Ref &r) : p(r.p), v(r.v){}
		inline Ref(P3f &p, P3f &v) : p(p), v(v){}
		
		inline void operator=(const Ref &r){ p = r.p;     v = r.v;      }
		inline void operator=(const Val &r){ p = r.first; v = r.second; }
		
		operator value_type() const{ return std::make_pair(p, v); }
		
		P3f &p, &v;
	};
	
	inline Ref operator*() const{ return Ref(*i1, *i2); }
	inline difference_type operator-(const Iter &I) const{ return i1-I.i1; }
	inline Iter &operator++() { ++i1; ++i2; return *this; }
	inline Iter &operator--() { --i1; --i2; return *this; }
	inline Iter &operator+=(difference_type d) { i1 += d; i2 += d; return *this; }
	inline Iter &operator-=(difference_type d) { i1 -= d; i2 -= d; return *this; }
	inline Iter operator+(difference_type d) const { return Iter{i1+d, i2+d}; }
	inline Iter operator-(difference_type d) const { return Iter{i1-d, i2-d}; }
	inline bool operator==(const Iter &I) const{ return i1 == I.i1; }
	inline bool operator!=(const Iter &I) const{ return i1 != I.i1; }
	inline bool operator>=(const Iter &I) const{ return i1 >= I.i1; }
	inline bool operator<=(const Iter &I) const{ return i1 <= I.i1; }
	inline bool operator> (const Iter &I) const{ return i1 >  I.i1; }
	inline bool operator< (const Iter &I) const{ return i1 <  I.i1; }
	
	P3f *i1, *i2;
};
namespace std{
template<> struct iterator_traits<Iter>
{
	typedef Iter::difference_type difference_type;
	typedef Iter::value_type      value_type;
	typedef Iter::Ref             reference;
	typedef void                  pointer; // not sure what this would have to be
	typedef std::random_access_iterator_tag iterator_category;
};}
inline void swap(const Iter::Ref &a, const Iter::Ref &b)
{
	std::swap(a.p, b.p);
	std::swap(a.v, b.v);
}

struct VCmp
{
	const P3f view;
	
	inline bool operator() (const P3f &a, const P3f &b){ return a*view > b*view; }
	
	inline bool operator() (const Iter::Val &a, const Iter::Val &b){ return a.first*view > b.first*view; }
	inline bool operator() (const Iter::Ref &a, const Iter::Ref &b){ return a.p*view     > b.p*view;     }
	inline bool operator() (const Iter::Ref &a, const Iter::Val &b){ return a.p*view     > b.first*view; }
	inline bool operator() (const Iter::Val &a, const Iter::Ref &b){ return a.first*view > b.p*view;     }
};

void GL_PointGraph::depth_sort(const P3f &view)
{
	if (!nvertexes || !needs_depth_sort()) return;

	if (graph.isVectorField())
	{
		// sort the points and vectors in lockstep
		P3f *p = pau.get(), *v = vau.get();
		std::sort(Iter{p,v}, Iter{p + nvertexes, v + nvertexes}, VCmp{view});
	}
	else
	{
		P3f *p = pau.get();
		std::sort(p, p + nvertexes, VCmp{view});
	}
}

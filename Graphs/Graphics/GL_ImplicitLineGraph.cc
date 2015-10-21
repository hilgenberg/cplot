#include "GL_ImplicitLineGraph.h"
#include "../Threading/ThreadMap.h"
#include <GL/gl.h>
#include <numeric>

//----------------------------------------------------------------------------------------------------------------------
// helper functions
//----------------------------------------------------------------------------------------------------------------------

static void contour(const P3d &A, const P3d &B, const P3d &C, ThreadInfo &ti, MemoryPool<P3f> &ls, MemoryPool<P3f> &ps)
{
	if (!defined(A.z) || !defined(B.z) || !defined(C.z)) return;
	int t = (A.z > 0.0) | ((B.z > 0.0) << 1) | ((C.z > 0.0) << 2);
	if (t == 0 || t == 7) return;
	const P3d &a = (t==1 || t==6) ? A : (t==2 || t==5) ? B : C; // the single point above/below zero
	const P3d &b = (t==1 || t==6) ? B : (t==2 || t==5) ? C : A;
	const P3d &c = (t==1 || t==6) ? C : (t==2 || t==5) ? A : B;

	P3d p1((a.x*b.z - a.z*b.x) / (b.z - a.z), (a.y*b.z - a.z*b.y) / (b.z - a.z), 0.0);
	P3d p2((a.x*c.z - a.z*c.x) / (c.z - a.z), (a.y*c.z - a.z*c.y) / (c.z - a.z), 0.0);

	if ((p1-p2).absq() < ti.is.min_lenq)
	{
		ti.ia.map((p1+p2)*0.5, *new(ps) P3f);
	}
	else
	{
		ti.ia.map(p1, *new(ls) P3f);
		ti.ia.map(p2, *new(ls) P3f);
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

static constexpr inline double sqr(double x){ return x*x; }
//static constexpr inline float  sqr(float  x){ return x*x; }

void GL_ImplicitLineGraph::update(int n_threads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------
	
	if (graph.type() != R2_R) return;

	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim != 1) return;
	
	DI_Axis ia(graph, true, false);
	if (ia.pixel <= 0.0) return;

	ic.vertex_normals = false;
	ic.face_normals   = false;
	ic.texture        = false;
	ic.do_grid        = false;
	
	DI_Subdivision is;
	is.detect_discontinuities = graph.options.disco;
	is.disco_limit = sqr(150.0 * ia.pixel / ia.range[0]);
	is.max_lenq    = sqr(  5.0 * ia.pixel / ia.range[0]);
	is.min_lenq    = sqr(std::max(1.0, graph.options.line_width) * ia.pixel);
	
	double q0 = graph.options.quality;
	size_t max_points = (size_t)((quality*q0*1000.0 + 1.0)*1000.0);
	int ngrid = (int)ceil(sqrt(max_points));
	if (ngrid < 12) ngrid = 12;
	DI_Grid ig(ia, 0.0, ngrid, false);
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&is);
	info.push_back(&ig);
	
	//------------------------------------------------------------------------------------------------------------------
	// calculate the base grid
	//------------------------------------------------------------------------------------------------------------------
	
	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	std::unique_ptr<P3d[]> vss(new P3d[nx*ny]);
	P3d *vs = vss.get();

	Task task(&info);
	WorkLayer *layer = new WorkLayer("base", &task, NULL, 0);
	int chunk = (ny+2*n_threads-1) / (2*n_threads);
	if (chunk < 1) chunk = 1;
	int nchunks = 0;
	
	for (int i1 = 0; i1 < ny; i1 += chunk)
	{
		int i2 = std::min(i1+chunk, ny);
		
		++nchunks;
		layer->add_unit([&,i1,i2,vs](void *ti)
		{
			for (int i = i1; i < i2; ++i)
			{
				double y = ig.y[i];
				for (int j = 0; j < nx; ++j)
				{
					double x = ig.x[j];
					vs[nx*i+j].set(x, y, (*(ThreadInfo*)ti).eval(x, y, false, j>0));
				}
			}
		});
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// run contouring
	//------------------------------------------------------------------------------------------------------------------
	
	std::vector<MemoryPool<P3f>> line_storage(nchunks), dot_storage(nchunks);
	std::vector<size_t>          npoints(nchunks), ndots(nchunks);
	
	layer = new WorkLayer("contour", &task, layer, 0, -1);
	for (int i1 = 0; i1+1 < ny; i1 += chunk)
	{
		int i2 = std::min(i1+chunk, ny-1);
		layer->add_unit([&,vs,i1,i2,nx,chunk](void *ti)
		{
			MemoryPool<P3f> &ls = line_storage[i1/chunk], &ps = dot_storage[i1/chunk];
			for (int i = i1; i < i2; ++i)
			{
				int I = nx*i;
				for (int j = 0; j+1 < nx; ++j, ++I)
				{
					if ((i+j) & 1)
					{
						contour(vs[I], vs[I+1], vs[I+nx], *(ThreadInfo*)ti, ls, ps);
						contour(vs[I+1], vs[I+nx+1], vs[I+nx], *(ThreadInfo*)ti, ls, ps);
					}
					else
					{
						contour(vs[I], vs[I+1], vs[I+nx+1], *(ThreadInfo*)ti, ls, ps);
						contour(vs[I], vs[I+nx+1], vs[I+nx], *(ThreadInfo*)ti, ls, ps);
					}
				}
			}
			
			// count the points in our storage
			npoints[i1/chunk] = ls.n_items();
			ndots  [i1/chunk] = ps.n_items();
			assert(npoints[i1/chunk] % 2 == 0);
		});
	}
		
	//------------------------------------------------------------------------------------------------------------------
	// map and transfer
	//------------------------------------------------------------------------------------------------------------------

	layer = new WorkLayer("alloc", &task, layer, 0, -1);
	layer->add_unit([&npoints,&ndots,this](void * /*ti*/)
	{
		lines.resize(std::accumulate(npoints.begin(), npoints.end(), 0));
		dots.resize(std::accumulate(ndots.begin(), ndots.end(), 0));
	});

	layer = new WorkLayer("transfer", &task, layer, 0, -1);
	for (int i = 0; i < nchunks; ++i)
	{
		layer->add_unit([&,i,this](void * /*ti*/)
		{
			P3f *dst = lines.points() + std::accumulate(npoints.begin(), npoints.begin()+i, 0);
			for (P3f &p : line_storage[i]) *dst++ = p;
			dst = dots.points() + std::accumulate(ndots.begin(), ndots.begin()+i, 0);
			for (P3f &p : dot_storage[i]) *dst++ = p;
		});
	}
	
	task.run(n_threads);
}

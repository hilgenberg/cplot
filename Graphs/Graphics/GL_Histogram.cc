#include "GL_Histogram.h"
#include "../OpenGL/GL_Mesh.h"
#include "../../Utility/MemoryPool.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "../OpenGL/GL_Util.h"
#include "GL_Graph.h"
#include "../Geometry/Vector.h"
#include "../OpenGL/GL_Image.h"
#include "../Geometry/Rotation.h"

#include <vector>
#include <algorithm>
#include <random>
#include <cassert>
#include <atomic>

//----------------------------------------------------------------------------------------------------------------------
// helper stuff
//----------------------------------------------------------------------------------------------------------------------

static std::mt19937 seed_rng;

//----------------------------------------------------------------------------------------------------------------------
// update worker
//----------------------------------------------------------------------------------------------------------------------

typedef std::atomic_uint_fast32_t AtomicCount;
typedef std::uint_fast32_t        Count;

static void update(ThreadInfo &ti, Count N, AtomicCount *count, int kx, int ky, bool normal, double scale, unsigned seed)
{
	const DI_Calc &ic = ti.ic;
	BoundContext  &ec = ti.ec;
	const DI_Axis &ia = ti.ia;
	
	std::uniform_real_distribution<> dist(-1.0, 1.0);
	std::mt19937 rng(seed);
	
	assert(kx >= 1 && ky >= 1);
	
	double x0 = ia.min[0], x1 = ia.max[0], xr = x1-x0;
	double y0 = ia.min[1], y1 = ia.max[1], yr = y1-y0;
	
	for (size_t i = 0; i < N; ++i)
	{
		//--- next random number ---------------------------------------------------------------------------------------
		
		double x, y, r;
		do{ x = dist(rng); y = dist(rng); r = x*x + y*y; }while (r >= 1.0);
		cnum z(x,y);
		if (normal)
		{
			z *= sqrt(-2.0*log(r) / r) * scale;
		}
		else
		{
			z *= scale;
		}
		
		//--- apply function -------------------------------------------------------------------------------------------
		
		if (ic.xi >= 0) ec.set_input(ic.xi, z);
		ec.eval();
		z = ec.output(0);
		
		//--- find out where it went -----------------------------------------------------------------------------------
		
		if (!defined(z))
		{
			++count[-1];
		}
		else
		{
			int i = (int)floor((z.real()-x0)/xr * kx);
			int j = (int)floor((z.imag()-y0)/yr * ky);
			if (i < 0 || j < 0 || i >= kx || j >= ky)
			{
				++count[-1];
			}
			else
			{
				++count[j*kx+i];
			}
		}
	}
}

static void geometry(ThreadInfo &ti, GL_Mesh &m, bool *edges, int kx, int ky,
					 Count N, double min, double max, float th, double *H)
{
	const DI_Axis &ia = ti.ia;
	assert(min < max && N > 0);
	
	// scale [0, max] into [H0,H1] by h = a*count + b  so that  a*0 + b = H0, a*max + b = H1
	const double H1 = ia.zh*(1.0 - 1e-4), H0 = ia.zh*(-1.0 + 0.01);
	const double a = (H1-H0)/cbrt(max), b = H0;
	float fx = std::min(1.0f, th), fy = std::min(1.0f, 1.0f/th);
	float z0 = (float)(ia.zh*(-1.0+1e-5));
	
	GLuint *f = m.faces  ();
	P3f    *n = m.normals();
	GLuint vi = 0;
	P3f    *v = m.points () + vi;
	P2f    *t = m.texture() + vi;
	
	for (int i = 0; i < ky; ++i)
	{
		P2f A(-1.0f, (float)(ia.yh*(2* i   -ky) / ky));
		P2f D(-1.0f, (float)(ia.yh*(2*(i+1)-ky) / ky));
		for (int j = 0; j < kx; ++j)
		{
			P2f B((float)(2*(j+1)-kx) / kx, (float)(ia.yh*(2* i   -ky) / ky));
			P2f C((float)(2*(j+1)-kx) / kx, (float)(ia.yh*(2*(i+1)-ky) / ky));
			
			assert(min <= *H && *H <= max);
			float h = (float)(b + a * cbrt(*H++));
			assert(h >= H0-1e-6f && h <= H1+1e-6f);
			
			*v++ = P3f(A.x, A.y, z0); t++ -> set(0.5f + 0.5f*fx*A.x, 0.5f + 0.5f*fy*A.y);
			*v++ = P3f(B.x, B.y, z0); t++ -> set(0.5f + 0.5f*fx*B.x, 0.5f + 0.5f*fy*B.y);
			*v++ = P3f(C.x, C.y, z0); t++ -> set(0.5f + 0.5f*fx*C.x, 0.5f + 0.5f*fy*C.y);
			*v++ = P3f(D.x, D.y, z0); t++ -> set(0.5f + 0.5f*fx*D.x, 0.5f + 0.5f*fy*D.y);
			*v++ = P3f(A.x, A.y, h);  t++ -> set(0.5f + 0.5f*fx*A.x, 0.5f + 0.5f*fy*A.y);
			*v++ = P3f(B.x, B.y, h);  t++ -> set(0.5f + 0.5f*fx*B.x, 0.5f + 0.5f*fy*B.y);
			*v++ = P3f(C.x, C.y, h);  t++ -> set(0.5f + 0.5f*fx*C.x, 0.5f + 0.5f*fy*C.y);
			*v++ = P3f(D.x, D.y, h);  t++ -> set(0.5f + 0.5f*fx*D.x, 0.5f + 0.5f*fy*D.y);
			
			// bottom
			*f++ = vi;   *f++ = vi+3; *f++ = vi+1;
			*f++ = vi+1; *f++ = vi+3; *f++ = vi+2;
			// top
			*f++ = vi+4; *f++ = vi+5; *f++ = vi+7;
			*f++ = vi+5; *f++ = vi+6; *f++ = vi+7;
			// front
			*f++ = vi;   *f++ = vi+1; *f++ = vi+4;
			*f++ = vi+1; *f++ = vi+5; *f++ = vi+4;
			// back
			*f++ = vi+3; *f++ = vi+7; *f++ = vi+2;
			*f++ = vi+2; *f++ = vi+7; *f++ = vi+6;
			// left
			*f++ = vi;   *f++ = vi+4; *f++ = vi+3;
			*f++ = vi+3; *f++ = vi+4; *f++ = vi+7;
			// right
			*f++ = vi+1; *f++ = vi+2; *f++ = vi+5;
			*f++ = vi+6; *f++ = vi+5; *f++ = vi+2;
			vi += 8;
			
			// avoid duplicate edges, so we can use the faster mesh.set_grid
			*edges++ = true;  *edges++ = false;  *edges++ = true;
			*edges++ = false; *edges++ = true;   *edges++ = true;
			*edges++ = true;  *edges++ = false;  *edges++ = true;
			*edges++ = true;  *edges++ = true;   *edges++ = false;
			*edges++ = false; *edges++ = false;  *edges++ = true;
			*edges++ = true;  *edges++ = false;  *edges++ = false;
			*edges++ = true;  *edges++ = false;  *edges++ = false;
			*edges++ = false; *edges++ = false;  *edges++ = true;
			*edges++ = false; *edges++ = false;  *edges++ = false;
			*edges++ = false; *edges++ = false;  *edges++ = false;
			*edges++ = false; *edges++ = false;  *edges++ = false;
			*edges++ = false; *edges++ = false;  *edges++ = false;
			
			n++ -> set( 0.0f,  0.0f, -1.0f); *n = n[-1]; ++n;
			n++ -> set( 0.0f,  0.0f,  1.0f); *n = n[-1]; ++n;
			n++ -> set( 0.0f, -1.0f,  0.0f); *n = n[-1]; ++n;
			n++ -> set( 0.0f,  1.0f,  0.0f); *n = n[-1]; ++n;
			n++ -> set(-1.0f,  0.0f,  0.0f); *n = n[-1]; ++n;
			n++ -> set( 1.0f,  0.0f,  0.0f); *n = n[-1]; ++n;
			
			A  = B;  D  = C;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

void GL_Histogram::update(int nthreads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	assert(graph.type() == C_C);
	assert(graph.isHistogram());
	bool normal = (graph.options.hist_mode == HM_Normal);
	
	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim != 1)
	{
		mesh.clear();
		return;
	}
	ic.vertex_normals = false; // always flat shaded
	ic.face_normals   = true;
	ic.texture        = true;
	ic.do_grid        = false; // no edge flags
	
	DI_Axis ia(graph, false, false);
	DI_Grid        ig;
	DI_Subdivision is;
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&is);
	info.push_back(&ig);
	
	//------------------------------------------------------------------------------------------------------------------
	// (2) computation parameters and run
	//------------------------------------------------------------------------------------------------------------------
	
	double q0 = graph.options.quality;
	Count N = (Count)(quality*q0*1e7)+200;
	if (nthreads < 1) nthreads = 1;
	N = ((N+nthreads-1) / nthreads) * nthreads;
	
	unsigned kx = (unsigned)graph.options.grid_density, ky = kx; // number of bins along each axis
	if (ia.range[0] >= ia.range[1])
	{
		ky = (unsigned)(kx * ia.range[1] / ia.range[0]);
	}
	else
	{
		kx = (unsigned)(ky * ia.range[0] / ia.range[1]);
	}
	kx |= 1; ky |= 1; // odd numbers are better on real functions (and this ensures kx,ky > 0)
	const unsigned kk = kx * ky;
	
	std::unique_ptr<AtomicCount> _counts(new AtomicCount[kk+1]); // 0 is number of undefined points
	AtomicCount *counts = _counts.get();
	for (size_t i = 0, n = kk+1; i < n; ++i) counts[i] = 0;
	++counts; // so undefineds get index -1
	
	#ifdef DEBUG
	{
		Count NN = counts[-1];
		for (size_t i = 0, n = kk; i < n; ++i) NN += counts[i];
		assert(NN == 0);
	}
	#endif
	
	std::vector<unsigned> seeds;
	for (int i = 0; i < nthreads; ++i) seeds.push_back(seed_rng());
	
	Task task(&info);
	WorkLayer *layer = new WorkLayer("calculate & count", &task, NULL);
	for (int i = 0; i < nthreads; ++i)
	{
		layer->add_unit([=,&seeds](void *ti)
		{
			::update(*(ThreadInfo*)ti, N / nthreads, counts, kx, ky, normal, graph.options.hist_scale, seeds[i]);
		});
	}
	
	task.run(nthreads);
	
	//------------------------------------------------------------------------------------------------------------------
	// (3) max and faces
	//------------------------------------------------------------------------------------------------------------------
	
	N -= counts[-1];
	
	#ifdef DEBUG
	{
		Count NN = 0;
		for (size_t i = 0, n = kk; i < n; ++i) NN += counts[i];
		assert(NN == N);
	}
	#endif
	
	std::vector<double> H(kk);
	double max = 0.0, min = N;
	for (size_t i = 0, n = kk; i < n; ++i)
	{
		double h = counts[i];
		if (h > max) max = h;
		if (h < min) min = h;
		H[i] = h;
	}
	_counts.reset(NULL); counts = NULL;
	
	if (min > max || max == 0 || N == 0)
	{
		mesh.clear();
		return;
	}
	
	mesh.resize(kk*8, kk*12, GL_Mesh::NormalMode::Face, true);
	std::unique_ptr<bool> edges(new bool[kk*12*3]);
	
	double th = (double)graph.options.texture.h() / graph.options.texture.w();
	
	Task task2(&info);
	WorkLayer *layer2 = new WorkLayer("geometry", &task2, NULL);
	for (int i = 0; i < 1; ++i)
	{
		layer2->add_unit([=,&H,&edges](void *ti)
		{
			::geometry(*(ThreadInfo*)ti, mesh, edges.get(), kx, ky, N, min, max, (float)th, H.data());
		});
	}
	
	task2.run(nthreads);
	mesh.set_grid(edges.get(), false);
}

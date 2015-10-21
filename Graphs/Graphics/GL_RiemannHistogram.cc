#include "GL_RiemannHistogram.h"
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

enum Sector
{
	xp = 0, xm = 1,
	yp = 2, ym = 3,
	zp = 4, zm = 5
};

static inline Sector sector(const P3f &v)
{
	float x = fabsf(v.x), y = fabsf(v.y), z = fabsf(v.z);
	if (x > y){ if (x > z) return v.x > 0.0f ? xp : xm; }
	else        if (y > z) return v.y > 0.0f ? yp : ym;
	return                        v.z > 0.0f ? zp : zm;
}

static inline P2d lambert(const P3f &p, Sector sector)
{
	double f;
	switch (sector)
	{
		case zm: f = sqrt(2.0/(1.0-p.z)); return P2d(p.x*f, -p.y*f);
		case zp: f = sqrt(2.0/(1.0+p.z)); return P2d(p.x*f,  p.y*f);

		case ym: f = sqrt(2.0/(1.0-p.y)); return P2d(p.x*f,  p.z*f);
		case yp: f = sqrt(2.0/(1.0+p.y)); return P2d(p.x*f, -p.z*f);

		case xm: f = sqrt(2.0/(1.0-p.x)); return P2d(p.y*f, -p.z*f);
		case xp: f = sqrt(2.0/(1.0+p.x)); return P2d(p.y*f,  p.z*f);
	}
	assert(false); throw std::logic_error("sector corrupted");
}
static inline P3f lambert(const P2d &p, Sector sector)
{
	double rq = 0.25*p.absq(), f0 = 1.0-rq, f1 = sqrt(f0); f0 -= rq;
	switch (sector)
	{
		case zm: return P3f((float)(f1*p.x), (float)(-f1*p.y), (float)-f0);
		case zp: return P3f((float)(f1*p.x), (float)( f1*p.y), (float) f0);

		case ym: return P3f((float)(f1*p.x), (float)-f0, (float)( f1*p.y));
		case yp: return P3f((float)(f1*p.x), (float) f0, (float)(-f1*p.y));

		case xm: return P3f((float)-f0, (float)(f1*p.x), (float)(-f1*p.y));
		case xp: return P3f((float) f0, (float)(f1*p.x), (float)( f1*p.y));
	}
	assert(false); throw std::logic_error("sector corrupted");
}

static inline P2f sector_coords(const P3f &p0, Sector &sector)
{
	P2d p = lambert(p0, sector = ::sector(p0));
	
	double x = fabs(p.x), y = fabs(p.y);
	if (y <= x)
	{
		double r = sqrt(2.0 * x*x + y*y), f = sqrt(r * (x+r) * 0.5);
		return P2f((float)copysign(f, p.x), (float)(atan2((r-x)*p.y, r*x + y*y) * f * 12.0/M_PI));
	}
	else
	{
		double r = sqrt(x*x + 2.0 * y*y), f = sqrt(r * (y+r) * 0.5);
		return P2f((float)(atan2((r-y)*p.x, r*y + x*x) * f * 12.0/M_PI), (float)copysign(f, p.y));
	}
}

static inline P3f sphere_coords(const P2f &p, Sector sector)
{
	assert(fabsf(p.x) < 1.0f+1e-6f);
	assert(fabsf(p.y) < 1.0f+1e-6f);

	if (fabsf(p.y) <= fabsf(p.x))
	{
		double sa, ca; sincos(p.y * M_PI / (12.0 * p.x), sa, ca);
		double f = p.x * sqrt(M_SQRT2 / (M_SQRT2 - ca));
		return lambert(P2d((M_SQRT2 * ca - 1.0) * f, M_SQRT2 * sa * f), sector);
	}
	else
	{
		double sa, ca; sincos(p.x * M_PI / (12.0 * p.y), sa, ca);
		double f = p.y * sqrt(M_SQRT2 / (M_SQRT2 - ca));
		return lambert(P2d(M_SQRT2 * sa * f, (M_SQRT2 * ca - 1.0) * f), sector);
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update worker
//----------------------------------------------------------------------------------------------------------------------

typedef std::atomic_uint_fast32_t AtomicCount;
typedef std::uint_fast32_t        Count;

static void update(ThreadInfo &ti, Count N, AtomicCount *count, int k, unsigned seed)
{
	const DI_Calc &ic = ti.ic;
	BoundContext  &ec = ti.ec;

	std::uniform_real_distribution<> dist(-1.0, 1.0);
	std::mt19937 rng(seed);
	
	assert(k >= 1);
	const int kk = k*k; assert(kk >= k);
	
	for (size_t i = 0; i < N; ++i)
	{
		//--- next random number ---------------------------------------------------------------------------------------
		
		P3d v; double d;
		do{ v.x = dist(rng); v.y = dist(rng); d = v.x*v.x + v.y*v.y; }while(d >= 1.0);
		v.z = 1.0 - 2.0*d; d = 2.0 * sqrt(1.0 - d); v.x *= d; v.y *= d;

		//--- apply function -------------------------------------------------------------------------------------------

		cnum z;
		riemann(v, z);
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
			P3f v;
			riemann(z, v);
			Sector s;
			P2f p = sector_coords(v, s);
			assert(fabsf(p.x) <= 1.0f && fabsf(p.y) <= 1.0f);
			int x = (int)(0.5f*(p.x+1.0f)*k); assert(x >= 0 && x <= k);
			if (x < 0) x = 0; if (x >= k) x = k-1;
			int y = (int)(0.5f*(p.y+1.0f)*k);  assert(y >= 0 && y <= k);
			if (y < 0) y = 0; if (y >= k) y = k-1;
			
			++count[kk*s + k*y + x];
		}
	}
}

static void geometry(ThreadInfo &/*unused*/, GL_Mesh &m, bool *edges, Sector sector, int k,
                     Count N, double min, double max, double th, double *H)
{
	assert(min < max && N > 0);
	
	// k = number of subdivisions of each side
	const int kk = k * k;
	H += kk * sector;
	
	// scale [0, cbrt max] into [H0,H1] by h = a*count + b  so that  a*0 + b = H0, a*cbrtmax + b = H1
	const double H1 = 1.0, H0 = 0.02;
	const double a = (H1-H0)/cbrt(max), b = H0;

	GLuint *f = m.faces  () + sector*kk*6*3;
	P3f    *n = m.normals() + sector*kk*6;
	GLuint vi = sector*kk*5;
	P3f    *v = m.points () + vi;
	P2f    *t = m.texture() + vi;
	edges    += sector*kk*6*3;
	
	for (int i = 0; i < k; ++i)
	{
		P2f A0(-1.0f, (float)(2* i   -k) / k);
		P2f D0(-1.0f, (float)(2*(i+1)-k) / k);
		P3f A(sphere_coords(A0, sector));
		P3f D(sphere_coords(D0, sector));
		for (int j = 0; j < k; ++j)
		{
			P2f B0((float)(2*(j+1)-k) / k, (float)(2* i   -k) / k);
			P2f C0((float)(2*(j+1)-k) / k, (float)(2*(i+1)-k) / k);
			P3f B(sphere_coords(B0, sector));
			P3f C(sphere_coords(C0, sector));

			assert(fabsf(A.abs()-1.0f) < 1e-4f);
			assert(fabsf(B.abs()-1.0f) < 1e-4f);
			assert(fabsf(C.abs()-1.0f) < 1e-4f);
			assert(fabsf(D.abs()-1.0f) < 1e-4f);
			
			assert(min <= *H && *H <= max);
			float h = (float)(b + a * cbrt(*H++));
			assert(h >= H0-1e-6f && h <= H1+1e-6f);

			*v++ = A * h;
			*v++ = B * h;
			*v++ = C * h;
			*v++ = D * h;
			v++ ->clear();

			*f++ = vi;   *f++ = vi+1; *f++ = vi+3;
			*f++ = vi+1; *f++ = vi+2; *f++ = vi+3;
			*f++ = vi  ; *f++ = vi+4; *f++ = vi+1;
			*f++ = vi+1; *f++ = vi+4; *f++ = vi+2;
			*f++ = vi+2; *f++ = vi+4; *f++ = vi+3;
			*f++ = vi+3; *f++ = vi+4; *f++ = vi;
			vi += 5;
			
			// avoid duplicate edges, so we can use the faster mesh.set_grid
			*edges++ = true;  *edges++ = false;  *edges++ = true;
			*edges++ = true;  *edges++ = true;   *edges++ = false;
			*edges++ = true;  *edges++ = false;  *edges++ = false;
			*edges++ = true;  *edges++ = false;  *edges++ = false;
			*edges++ = true;  *edges++ = false;  *edges++ = false;
			*edges++ = true;  *edges++ = false;  *edges++ = false;
			
			cross(*n, B-A, D-A); n->to_unit(); n[1] = n[0]; n += 2;
			cross(*n, B, A); n->to_unit(); ++n;
			cross(*n, C, B); n->to_unit(); ++n;
			cross(*n, D, C); n->to_unit(); ++n;
			cross(*n, A, D); n->to_unit(); ++n;

			P2f *Ps[5] = {&A0, &B0, &C0, &D0};
			double fx = std::min(1.0, th), fy = std::min(1.0, 1.0/th);
			for (int ip = 0; ip < 4; ++ip)
			{
				P2f &P = *Ps[ip];
				
				switch (sector)
				{
					case xp:
					case xm:
						t->x = (float)(0.5f + 0.5f*fx*P.y);
						t->y = (float)(0.5f - 0.5f*fy*P.x);
						break;
						
					case yp:
					case ym:
						t->x = (float)(0.5f + 0.5f*fx*P.x);
						t->y = (float)(0.5f + 0.5f*fy*P.y);
						break;

					case zp:
					case zm:
						t->x = (float)(0.5f + 0.5f*fx*P.x);
						t->y = (float)(0.5f - 0.5f*fy*P.y);
						break;
				}
				++t;
			}
			*t = (t[-2]+t[-4])*0.5f; ++t;

			A  = B;  D  = C;
			A0 = B0; D0 = C0;
		}
	}
	
	#ifdef DEBUG
	f = m.faces() + sector*kk*6*3;
	for (int i = 0, n = kk*6*3; i < n; ++i)
	{
		assert(f[i] < (unsigned)kk*5*(sector+1));
		assert(f[i] >= (unsigned)kk*5*sector);
	}
	#endif
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

void GL_RiemannHistogram::update(int nthreads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	assert(graph.type() == C_C);
	assert(graph.isHistogram());
	
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
	
	DI_Axis ia(graph, false, false); // unused
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

	unsigned k = (unsigned)graph.options.grid_density; // divide each cube edge into subdiv pieces
	k |= 1; // odd numbers are better on real functions (and this ensures k > 0)
	const unsigned kk = k*k;
	
	std::unique_ptr<AtomicCount> _counts(new AtomicCount[kk*6+1]); // 0 is number of undefined points
	AtomicCount *counts = _counts.get();
	//for (size_t i = 0, n = k*k*6+1; i < n; ++i) atomic_init(counts+i, (Count)0);
	for (size_t i = 0, n = kk*6+1; i < n; ++i) counts[i] = 0;
	++counts; // so undefineds get index -1

	#ifdef DEBUG
	{
		Count NN = counts[-1];
		for (size_t i = 0, n = kk*6; i < n; ++i) NN += counts[i];
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
			::update(*(ThreadInfo*)ti, N / nthreads, counts, k, seeds[i]);
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
		for (size_t i = 0, n = kk*6; i < n; ++i) NN += counts[i];
		assert(NN == N);
	}
	#endif

	std::vector<double> H(kk*6);
	double max = 0.0, min = N;
	for (size_t i = 0, n = kk*6; i < n; ++i)
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
	
	mesh.resize(k*k*6*5, k*k*6*6, GL_Mesh::NormalMode::Face, true);
	std::unique_ptr<bool> edges(new bool[kk*6*6*3]);

	double th = (double)graph.options.texture.h() / graph.options.texture.w();

	Task task2(&info);
	WorkLayer *layer2 = new WorkLayer("geometry", &task2, NULL);
	for (int i = 0; i < 6; ++i)
	{
		layer2->add_unit([=,&H,&edges](void *ti)
		{
			::geometry(*(ThreadInfo*)ti, mesh, edges.get(), (Sector)i, k, N, min, max, th, H.data());
		});
	}
	
	task2.run(nthreads);
	mesh.set_grid(edges.get(), false);
}

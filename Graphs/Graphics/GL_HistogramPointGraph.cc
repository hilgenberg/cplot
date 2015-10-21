#include "GL_HistogramPointGraph.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "../OpenGL/GL_Util.h"
#include "../Geometry/Vector.h"
#include "../Graph.h"

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

static void update(ThreadInfo &ti, P3f *p, size_t N, size_t &skipped, bool normal, double scale, unsigned seed)
{
	const DI_Calc &ic = ti.ic;
	BoundContext  &ec = ti.ec;
	const DI_Axis &ia = ti.ia;
	
	std::uniform_real_distribution<> dist(-1.0, 1.0);
	std::mt19937 rng(seed);
	
	double x0 = ia.center[0], xr = ia.range[0];
	double y0 = ia.center[1], yr = ia.yh;
	
	skipped = 0;
	
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
			++skipped;
		}
		else
		{
			x = (z.real() - x0)/xr;
			y = (z.imag() - y0)/xr;
			if (x < -1.0 || x > 1.0 || y < -yr || y > yr)
			{
				++skipped;
			}
			else
			{
				*p++ = P3f((float)x, (float)y, 0.0f);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

void GL_HistogramPointGraph::update(int nthreads, double quality)
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
		nvertexes = 0;
		pau.reset(NULL);
		vau.reset(NULL);
		return;
	}
	
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
	size_t N = (size_t)(quality*q0*1e5)+200;
	if (nthreads < 1) nthreads = 1;
	N = ((N+nthreads-1) / nthreads) * nthreads;
	nvertexes = N;
	pau.reset(new P3f[N]);
	vau.reset(NULL);
	P3f *p = pau.get();
	
	std::vector<unsigned> seeds;
	for (int i = 0; i < nthreads; ++i) seeds.push_back(seed_rng());
	std::vector<size_t> skipped(nthreads, 0);
	
	Task task(&info);
	WorkLayer *layer = new WorkLayer("collect", &task, NULL);
	for (int i = 0; i < nthreads; ++i)
	{
		layer->add_unit([=,&seeds,&skipped](void *ti)
		{
			::update(*(ThreadInfo*)ti, p, N / nthreads, skipped[i], normal, graph.options.hist_scale, seeds[i]);
		});
		p += N / nthreads;
	}
	
	task.run(nthreads);
	
	size_t offset = 0;
	p = pau.get();
	for (int i = 0; i < nthreads; ++i, p += N / nthreads)
	{
		size_t skip = skipped[i];
		if (offset) memmove(p-offset, p, sizeof(P3f)*(N/nthreads - skip));
		nvertexes -= skip;
		offset += skip;
	}
}

#include "GL_RiemannHistogramPointGraph.h"
#include "../../Utility/MemoryPool.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "../OpenGL/GL_Util.h"
#include "../Geometry/Vector.h"
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

static void update(ThreadInfo &ti, P3f *p, size_t N, unsigned seed)
{
	const DI_Calc &ic = ti.ic;
	BoundContext  &ec = ti.ec;
	
	std::uniform_real_distribution<> dist(-1.0, 1.0);
	std::mt19937 rng(seed);
	
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
			*p++ = (P3f)(v*0.1);
		}
		else
		{
			riemann(z, *p++);
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

void GL_RiemannHistogramPointGraph::update(int nthreads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	assert(graph.type() == C_C);
	assert(graph.isHistogram());
	
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
	
	Task task(&info);
	WorkLayer *layer = new WorkLayer("collect", &task, NULL);
	for (int i = 0; i < nthreads; ++i)
	{
		layer->add_unit([=,&seeds](void *ti)
		{
			::update(*(ThreadInfo*)ti, p, N / nthreads, seeds[i]);
		});
		p += N / nthreads;
	}
	
	task.run(nthreads);
}

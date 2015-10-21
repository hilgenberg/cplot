#pragma once
#include "VisibilityFlags.h"
#include "Vector.h"
#include <vector>
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include <string>

//----------------------------------------------------------------------------------------------------------------------
// Vertex
//----------------------------------------------------------------------------------------------------------------------

struct Vertex2D
{
	POOL_ITEM(Vertex2D);

	double   u, v;     // variable values
	P3f      p;        // axis-mapped, in [-1,1] coordinates
	P3f      normal;   // vertex normal
	P2f      texture;  // texture coordinates
	unsigned gl_index; // index in the gl display list during drawing
	bool     exists;   // point could be calculated / function is defined
	bool     uglued, vglued; // one of the points where S1 x S1 is glued together?
	//#ifdef DEBUG
	//void *marker; // remember who counted this, so we can be sure it's the same thread that copies it to the GL array
	//#endif
	
	VisibilityFlags vis;

	char padding[2]; // to 64 bits

	static const unsigned npos    = 0xFFFFFFFF;
	static const unsigned counted = 0xFFFFFFFE;

	Vertex2D(double u, double v, ThreadInfo &ti) : gl_index(npos), uglued(false), vglued(false), u(u), v(v)
	{
		memset(normal, 0, sizeof(P3f));
		calc(ti);
		if (ti.ic.texture && exists) ti.ia.map_texture(u, v, texture);
	}

	// create a midpoint
	Vertex2D(const Vertex2D &a, const Vertex2D &b, ThreadInfo &ti) : gl_index(npos), uglued(a.uglued && b.uglued), vglued(a.vglued && b.vglued)
	{
		u = 0.5 * (a.u + b.u);
		v = 0.5 * (a.v + b.v);
		if (a.uglued != b.uglued && fabs(a.u - b.u) > M_PI) u += M_PI;
		if (a.vglued != b.vglued && fabs(a.v - b.v) > M_PI) v += M_PI;
		memset(normal, 0, sizeof(P3f));
		calc(ti);
	}

private:
	void calc(ThreadInfo &ti, bool same_u = false, bool same_v = false)
	{
		assert(gl_index == npos);
		ti.eval(u, v, p, exists, same_u, same_v);
		if (exists)
		{
			vis.set(ti.ia, p);
			ti.ia.map_texture(u, v, texture);
		}
		else
		{
			vis.set_invalid();
		}
	}
};

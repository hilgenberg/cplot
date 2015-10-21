#pragma once

#include "Vertex2D.h"
struct Face;

#ifdef DEBUG
#define REASON(x) ++is.x
#define REASON2(s,x) if (s) ++is.x
#else
#define REASON(x)
#define REASON2(s,x)
#endif

struct Edge
{
	POOL_ITEM(Edge);

	Vertex2D * const a, * const b, *m;
	Edge *la, *lb; // winged edges, turning left at a and b (which is all we need to traverse every face ccw)
	Edge *ca, *cb; // non-null if the edge is split, ca goes from a to m, cb from m to b
	
	const bool  draw;
	bool  disco_a, disco_b; // is a-m or m-b a discontinuity?
	char padding[5]; // 64 bytes
	
	inline bool split() const{ return ca; }
	inline bool exists() const // true if either half exists
	{
		return m && m->exists && (a->exists || b->exists);
	}
	inline bool visible() const // true if either half is visible
	{
		return exists() && (a->exists && VisibilityFlags::visible(a->vis, m->vis) || b->exists && VisibilityFlags::visible(b->vis, m->vis));
	}
	inline float lenq() const{ return distq(a->p, b->p); }

	Edge(int depth, Vertex2D *a_, Vertex2D *b_, bool draw_, RawMemoryPool &pool, ThreadInfo &ti)
	: a(a_), b(b_), draw(draw_), la(NULL), lb(NULL)
	{
		assert(a && b);
		if (!a->exists && !b->exists)
		{
			m = NULL; // this is the only case when m is NULL, so the edge exists iff m is not NULL
			ca = cb = NULL;
			return;
		}
		
		m = new(pool) Vertex2D(*a, *b, ti);
		const DI_Subdivision &is = ti.is;
		bool divide;
		
		if (depth <= 0 /*|| m->exists && !VisibilityFlags::visible(a->vis, b->vis, m->vis)*/)
		{
			divide = false;
		}
		else if (!a->exists || !b->exists || !m->exists)
		{
			REASON(ssb_eodef);
			divide = true;
		}
		else
		{
			float lenq = distq(a->p, b->p);
			if (lenq > is.max_lenq)
			{
				divide = true;
				REASON(ssb_len);
			}
			else
			{
				float hq = distq(m->p, (a->p + b->p)*0.5f);
				divide = (hq > is.max_kink);
				REASON2(divide, ssb_kink);
			}
		}
		
		if (divide)
		{
			--depth;
			ca = new(pool) Edge(depth, a, m, draw, pool, ti);
			cb = new(pool) Edge(depth, m, b, draw, pool, ti);
		}
		else
		{
			ca = cb = NULL;
		}
	}
};

#undef REASON
#undef REASON2


inline void connect(Edge *e1, bool fwd1, Edge *e2, bool fwd2, Edge *e3, bool fwd3)
{
	assert((fwd1 ? e1->b : e1->a) == (fwd2 ? e2->a : e2->b));
	assert((fwd2 ? e2->b : e2->a) == (fwd3 ? e3->a : e3->b));
	assert((fwd3 ? e3->b : e3->a) == (fwd1 ? e1->a : e1->b));
	(fwd1 ? e1->lb : e1->la) = e2;
	(fwd2 ? e2->lb : e2->la) = e3;
	(fwd3 ? e3->lb : e3->la) = e1;
}


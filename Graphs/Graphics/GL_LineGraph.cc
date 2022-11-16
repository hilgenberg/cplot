#include "GL_LineGraph.h"
#include "Info.h"
#include "../Threading/ThreadMap.h"
#include "VisibilityFlags.h"
#include "../../Utility/Preferences.h"
#include <GL/gl.h>

//----------------------------------------------------------------------------------------------------------------------
// Vertex
//----------------------------------------------------------------------------------------------------------------------

struct Vertex
{
	POOL_ITEM(Vertex);
	Vertex(){ }
	
	double t; // input var value
	P3f    p; // p = f(t)
	
	bool            exists; // function was defined?
	VisibilityFlags vis;    // outside the visible field?
	
	inline void calc(ThreadInfo &ti)
	{
		ti.eval(t, p, exists);
		if (exists) vis.set(ti.ia, p); else vis.set_invalid();
	}
};

//----------------------------------------------------------------------------------------------------------------------
// VertexNode - for building linked lists of Vertexes in a MemoryPool
//----------------------------------------------------------------------------------------------------------------------

struct VertexNode
{
	POOL_ITEM(VertexNode);
	VertexNode() : next(NULL), prev(NULL), connect(false){ }
	
	Vertex v;
	VertexNode *prev, *next;
	
	bool connect; // draw a line from v to v.next?

	// cos of the angle in prev--v--next
	// undefined if any Vertex does not v.exist
	inline float cosphi() const
	{
		assert(prev && next);
		P3f d1, d2;
		sub(d1, prev->v.p, v.p);
		sub(d2, next->v.p, v.p);
		
		return d1*d2 / sqrtf( d1.absq() * d2.absq() );
	}
	inline float lenq() const
	{
		assert(next);
		return distq(next->v.p, v.p);
	}
};

//----------------------------------------------------------------------------------------------------------------------
// subdivide - subdivide between vn and vn.next as needed, set vn.connect
//----------------------------------------------------------------------------------------------------------------------

static void subdivide(VertexNode &vn, int depth, ThreadInfo &ti, MemoryPool<VertexNode> &pool)
{
	const DI_Subdivision &is = ti.is;
	
subdiv_start:{
	
	assert(vn.next);
	const Vertex &v1 = vn.v, &v2 = vn.next->v;

	if ((!v1.exists && !v2.exists) || (v1.exists && v2.exists && v1.vis*v2.vis))
	{
		vn.connect = false;
		return;
	}
	
	// find out if this needs subdivision
	if (depth && v1.t < v2.t) // never divide between the S1-connection from 2π back to 0
	{
		double lenq;
		if ((!v1.exists || !v2.exists) || // always divide these to get closest to the undefined point
				  ((lenq = vn.lenq()) > is.max_lenq) || // check length
				  (vn.prev && lenq > is.min_lenq && // check for bends
				   (vn.cosphi() > -1.0 + 0.004 || vn.next->next && vn.next->cosphi() > -1.0 + 0.004)))
		{
			VertexNode *m = new(pool) VertexNode;
			m->prev = &vn;
			m->next = vn.next;
			vn.next = m;
			if (m->next) m->next->prev = m;

			m->v.t = 0.5*(v1.t + v2.t);
			m->v.calc(ti);

			--depth;
			
			if (depth == 0 && is.detect_discontinuities && m->v.exists && v1.exists && v2.exists)
			{
				float l1 = vn.lenq(), l2 = m->lenq();
				if (l2 > 10000.0f*l1)
				{
					vn.connect = (l1 < is.disco_limit && !(v1.vis*m->v.vis));
					m->connect = false;
					return;
				}
				if (l1 > 10000.0f*l2)
				{
					vn.connect = false;
					m->connect = (l1 < is.disco_limit && !(v2.vis*m->v.vis));
					return;
				}
			}

			subdivide(*m, depth, ti, pool);
			goto subdiv_start; // avoids one call... why not.
		}
	}
	
	// don't divide, find out if we should connect
	vn.connect = (v1.exists && v2.exists && (!is.detect_discontinuities || vn.lenq() < is.disco_limit));
}
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

static constexpr inline double sqr(double x){ return x*x; }
//static constexpr inline float  sqr(float  x){ return x*x; }

void GL_LineGraph::update(int n_threads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------

	if (graph.plotvars().size() != 1) return;

	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim <= 0 || ic.dim > 3) return;
	
	bool circle = false, parametric = false;
	ic.embed_XZ = false;
	switch (graph.type())
	{
		case  R_R:  ic.embed_XZ = true; break;
		case S1_R2: circle = parametric = true;  break;
		case S1_R3: circle = parametric = true;  break;
		case  R_R2: parametric = true;  break;
		case  R_R3: parametric = true;  break;
		default: return;
	}
	
	DI_Axis ia(graph, !parametric, circle);
	if (ia.pixel <= 0.0) return;
	if (ia.is2D) ic.embed_XZ = false;
	
	DI_Subdivision is;
	is.max_lenq    = sqr(  5.0 * ia.pixel / ia.range[0]);
	is.min_lenq    = sqr(  1.0 * ia.pixel / ia.range[0]);
	is.disco_limit = sqr(150.0 * ia.pixel / ia.range[0]);
	is.detect_discontinuities = graph.options.disco;
	
	DI_Grid dummy;
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&is);
	info.push_back(&dummy);

	double q0 = graph.options.quality;
	int N = (int)((quality*q0*q0*5000.0 + 1.0)*100.0);
	
	double dyn = 0.25;
	is.max_lenq  = sqr(1.0 + dyn) * 8.0 / N / N; // f * sqrt(2) * 2 / N squared, f = 1+dyn
	int n = (int)ceil(N * (1.0*(1.0-dyn) + 0.01*dyn));
	if (n < 50) n = 50;
	
	int depth = (int) (ceil(log2((double)N / n)*1.5 + 4)*dyn);
	if (depth <  0) depth = 0;
	if (depth > 20) depth = 20;
	if (is.detect_discontinuities && depth < 1) depth = 1;

	//------------------------------------------------------------------------------------------------------------------
	// build the non-subdivided segment
	//------------------------------------------------------------------------------------------------------------------

	std::unique_ptr<VertexNode[]> vss(new VertexNode[n]);
	VertexNode *vs = vss.get();
	Task task(&info);
	WorkLayer *layer = new WorkLayer("base", &task, NULL, 0);
	int chunk = (n+2*n_threads-1) / (2*n_threads);
	if (chunk < 1) chunk = 1;
	int nchunks = 0;
	
	double t0 = ia.in_min[0], t1 = ia.in_max[0];

	for (int i1 = 0; i1 < n; i1 += chunk)
	{
		int i2 = std::min(i1+chunk, n);
		++nchunks;
		layer->add_unit([&,i1,i2](void *ti)
		{
			for (int i = i1; i < i2; ++i)
			{
				VertexNode &v = vs[i];
				v.next = i+1==n ? NULL : vs+i+1;
				v.prev =  i ==0 ? NULL : vs+i-1;

				v.v.t = t0 + i * (t1-t0) / (n-1);
				v.v.calc(*(ThreadInfo*)ti);
			}
			if (circle)
			{
				if (i1 == 0) vs[ 0 ].prev = vs+n-1;
				if (i2 == n) vs[n-1].next = vs;
			}
		});
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// subdivide
	//------------------------------------------------------------------------------------------------------------------

	if (!circle) vs[n-1].connect = false;
	std::vector<MemoryPool<VertexNode>> storage(nchunks);
	
	layer = new WorkLayer("subdivide", &task, layer, 1, -1);
	int m = n - (circle ? 0 : 1);
	for (int i1 = 0; i1 < m; i1 += chunk)
	{
		int i2 = std::min(i1+chunk, m);
		layer->add_unit([&,i1,i2](void *ti)
		{
			for (int i = i1; i < i2; ++i)
			{
				subdivide(vs[i], depth, *(ThreadInfo*)ti, storage[i1/chunk]);
			}
		});
	}
	
	task.run(n_threads);

	//------------------------------------------------------------------------------------------------------------------
	// minimize and count the segments
	//------------------------------------------------------------------------------------------------------------------

	size_t nv = 0, nd = 0, sl = 0;
	std::vector<size_t> segments;
	bool in = false, first = true;
	for (VertexNode *v = vs; v; v = v->next, first = false)
	{
		// minimize
		while (v->connect && v->next != vs && v->lenq() < is.min_lenq)
		{
			// v -- n -- (nn)
			v->connect = v->next->connect;
			v->next = v->next->next;
			//if (v->next) v->next->prev = v;
			
			if (v->next == v)
			{
				assert(v == vs);
				v->next = v->prev = NULL;
				v->connect = false;
			}
		}
		
		if (v->connect) // extend or start segment
		{
			++sl;
			in = true;
		}
		else if (in) // finish current segment
		{
			++sl;
			nv += sl;
			segments.push_back(sl);
			sl = 0;
			in = false;
		}
		else if (v->v.exists) // add to isolated points
		{
			// don't minimize isolated points - they should be far enough
			// apart because otherwise they would be connected
			++nd;
		}

		if (circle && v == vs && !first) break;
	}
	if (in)
	{
		// finish last segment
		nv += sl;
		segments.push_back(sl);
	}

	//------------------------------------------------------------------------------------------------------------------
	// transfer
	//------------------------------------------------------------------------------------------------------------------

	lines.resize(nv, segments);
	dots.resize(nd);
	P3f *pa = lines.points();
	P3f *pb = dots.points();

	in = false; first = true;
	for (VertexNode *v = vs; v; v = v->next, first = false)
	{
		if (v->connect)
		{
			// extend or start segment
			*pa++ = v->v.p;
			in = true;
		}
		else if (in)
		{
			// finish current segment
			*pa++ = v->v.p;
			in = false;
		}
		else if (v->v.exists)
		{
			// add to isolated points
			*pb++ = v->v.p;
		}
		
		if (circle && v == vs && !first) break;
	}
}

Opacity GL_LineGraph::opacity() const
{
	return graph.options.line_color.opaque() ? ANTIALIASED : TRANSPARENT;
}

bool GL_LineGraph::needs_depth_sort() const
{
	return graph.plot.options.aa_mode == AA_Lines || !graph.options.line_color.opaque();
}

void GL_LineGraph::draw(GL_RM &) const
{
	start_drawing();
	
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(graph.options.line_color.opaque() && !glIsEnabled(GL_LINE_SMOOTH));
	graph.options.line_color.set();
	glLineWidth((float)graph.options.line_width);
	glPointSize((float)graph.options.line_width);

	lines.draw();
	dots.draw();
	
	bool debug = Preferences::drawNormals();
	if (debug)
	{
		glDepthMask(!glIsEnabled(GL_LINE_SMOOTH));
		glPointSize(2.0f*(float)graph.options.line_width);
		glColor3d(1.0, 0.0, 0.0);
		lines.draw_dots();
		glColor3d(0.0, 1.0, 0.0);
		dots.draw();
	}

	finish_drawing();
}

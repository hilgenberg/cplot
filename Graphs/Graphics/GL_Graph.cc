#include "GL_Graph.h"
#include "../Graph.h"
#include "../Plot.h"
#include "../Geometry/Axis.h"
#include "../OpenGL/GL_Context.h"

static inline int max_planes()
{
	static int N = 0;
	if (!N) glGetIntegerv(GL_MAX_CLIP_PLANES, &N);
	assert(N >= 6); // 6 should be minimum
	return N;
}

static inline bool clip(const Graph &graph, int &first_plane_id)
{
	if (!graph.clipping()) return false;
	bool ccp = graph.plot.options.clip.on();
	if (ccp && max_planes() <= 6) return false;
	first_plane_id = GL_CLIP_PLANE0 + ccp;
	return true;
}

void GL_Graph::start_drawing() const
{
	GL_CHECK;
	
	int P = -1;
	const Axis &axis = graph.plot.axis;
	if (axis.type() == Axis::Box && clip(graph, P))
	{
		assert(P >= GL_CLIP_PLANE0);
		P4d cf;
		double r;

		r = fabs(axis.range(2) / axis.range(0)) + 1e-4;
		cf.set(0, 0, -1, r); glClipPlane(P, cf); glEnable(P++);
		cf.set(0, 0,  1, r); glClipPlane(P, cf); glEnable(P++);

		if (!graph.isGraph())
		{
			r = 1.0 + 1e-4; // +1e-4 saves the outermost grid lines of a graph from being clipped
			cf.set(-1, 0, 0, r); glClipPlane(P, cf); glEnable(P++);
			cf.set( 1, 0, 0, r); glClipPlane(P, cf); glEnable(P++);
			
			r = fabs(axis.range(1) / axis.range(0)) + 1e-4;
			cf.set(0, -1, 0, r); glClipPlane(P, cf); glEnable(P++);
			cf.set(0,  1, 0, r); glClipPlane(P, cf); glEnable(P++);
		}
	}

	GL_CHECK;
}

void GL_Graph::finish_drawing() const
{
	GL_CHECK;

	int P = -1;
	if (graph.plot.axis.type() == Axis::Box && clip(graph, P))
	{
		assert(P >= GL_CLIP_PLANE0);
		for (int i = 0; i < 6; ++i) glDisable(P+i);
	}

	glDepthMask(GL_TRUE);

	GL_CHECK;
}

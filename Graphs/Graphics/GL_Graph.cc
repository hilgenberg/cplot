#include "GL_Graph.h"
#include "../Graph.h"
#include "../Plot.h"
#include "../Geometry/Axis.h"
#include "../OpenGL/GL_Context.h"

static inline bool clip(const Graph &graph)
{
	return !graph.plot.options.clip.on() && graph.clipping();
}

void GL_Graph::start_drawing() const
{
	GL_CHECK;
	
	const Axis &axis = graph.plot.axis;
	if (axis.type() == Axis::Box && clip(graph))
	{
		P4d cf;
		double r;

		r = fabs(axis.range(2) / axis.range(0)) + 1e-4;
		cf.set(0, 0, -1, r); glClipPlane(GL_CLIP_PLANE0, cf); glEnable(GL_CLIP_PLANE0);
		cf.set(0, 0,  1, r); glClipPlane(GL_CLIP_PLANE1, cf); glEnable(GL_CLIP_PLANE1);

		if (!graph.isGraph())
		{
			r = 1.0 + 1e-4; // +1e-4 saves the outermost grid lines of a graph from being clipped
			cf.set(-1, 0, 0, r); glClipPlane(GL_CLIP_PLANE2, cf); glEnable(GL_CLIP_PLANE2);
			cf.set( 1, 0, 0, r); glClipPlane(GL_CLIP_PLANE3, cf); glEnable(GL_CLIP_PLANE3);
			
			r = fabs(axis.range(1) / axis.range(0)) + 1e-4;
			cf.set(0, -1, 0, r); glClipPlane(GL_CLIP_PLANE4, cf); glEnable(GL_CLIP_PLANE4);
			cf.set(0,  1, 0, r); glClipPlane(GL_CLIP_PLANE5, cf); glEnable(GL_CLIP_PLANE5);
		}
	}

	GL_CHECK;
}

void GL_Graph::finish_drawing() const
{
	GL_CHECK;

	if (graph.plot.axis.type() == Axis::Box && clip(graph))
	{
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
		glDisable(GL_CLIP_PLANE4);
		glDisable(GL_CLIP_PLANE5);
	}

	glDepthMask(GL_TRUE);

	GL_CHECK;
}

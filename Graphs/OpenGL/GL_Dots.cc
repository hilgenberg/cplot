#include "GL_Dots.h"
#include <GL/gl.h>
#include <algorithm>

void GL_Dots::resize(size_t np)
{
	if (n_points != np)
	{
		n_points = 0;
		p.reset(nullptr);
		p.reset(new P3f[np]);
		n_points = np;
	}
}

void GL_Dots::draw() const
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, p.get());
	glDrawArrays(GL_POINTS, (GLint)0, (GLsizei)n_points);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void GL_Dots::depth_sort(const P3f &view)
{
	if (!n_points) return;
	P3f *p = points();
	std::sort(p, p + n_points, [&view](const P3f &a, const P3f &b)->bool
	{
		return a*view > b*view;
	});
}

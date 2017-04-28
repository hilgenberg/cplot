#include "GL_Lines.h"
#include <GL/gl.h>
#include <numeric>
#include <algorithm>

void GL_Lines::resize(size_t np, const std::vector<size_t> &segments)
{
	s = segments;
	if (n_points != np)
	{
		n_points = 0;
		p.reset(nullptr);
		p.reset(new P3f[np]);
		n_points = np;
	}
	
#ifdef DEBUG
	size_t N = 0;
	for (size_t n : s) N += n;
	assert(N == np);
#endif
}

void GL_Lines::resize(size_t np)
{
	s.clear();
	if (n_points != np)
	{
		n_points = 0;
		p.reset(nullptr);
		p.reset(new P3f[np]);
		n_points = np;
	}
	assert(np % 2 == 0);
}

void GL_Lines::draw() const
{
	if (!n_points) return;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, p.get());
	
	if (s.empty())
	{
		glDrawArrays(GL_LINES, 0, (GLsizei)n_points);
	}
	else
	{
		size_t i = 0;
		for (size_t n : s)
		{
			glDrawArrays(GL_LINE_STRIP, (GLint)i, (GLsizei)n);
			i += n;
		}
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}

void GL_Lines::draw_dots() const
{
	if (!n_points) return;
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, p.get());
	
	if (s.empty())
	{
		glDrawArrays(GL_POINTS, 0, (GLsizei)n_points);
	}
	else
	{
		glDrawArrays(GL_POINTS, 0, (GLsizei)std::accumulate(s.begin(), s.end(), 0ULL));
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void GL_Lines::depth_sort(const P3f &view)
{
	if (!n_points || !s.empty()) return;

	struct Seg{ P3f A,B; };
	Seg *p = (Seg*)points();

	std::sort(p, p + n_points/2, [&view](const Seg &a, const Seg &b)->bool
	{
		return (a.A+a.B)*view > (b.A+b.B)*view;
	});
}

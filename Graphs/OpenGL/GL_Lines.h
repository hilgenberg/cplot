#pragma once
#include "../Geometry/Vector.h"
#include <memory>
#include <vector>

/**
 * GL_Lines is a collection of line strips.
 */

class GL_Lines
{
public:
	GL_Lines() : n_points(0){ }
	
	GL_Lines(const GL_Lines &m) = delete;
	GL_Lines &operator=(const GL_Lines &) = delete;

	void resize(size_t n_points, const std::vector<size_t> &segments); // --> GL_LINE_STRIP
	void resize(size_t n_points); // series of lines (p0-p1  p2-p3  p4-p5 ...) --> GL_LINES
	
	void draw() const;
	void draw_dots() const;
	void depth_sort(const P3f &view); // only for GL_LINES, not for GL_LINE_STRIP
	
	P3f       *points()       { return p.get(); }
	const P3f *points () const{ return p.get(); }
	
private:
	// p[0..s0-1], p[s0..s1-1], ... p[s[n-2], s[n-1]-1] are the line segments unless
	// s is empty, which makes p[0..1], p[2..3], ... p[n-2..n-1] the line segments
	std::vector<size_t>    s;
	
	std::unique_ptr<P3f[]> p; // points
	size_t n_points; // size of p
};

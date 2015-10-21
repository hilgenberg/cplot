#pragma once
#include "../Geometry/Vector.h"
#include <memory>

/**
 * GL_Dots is a collection of isolated points.
 */

class GL_Dots
{
public:
	GL_Dots() : n_points(0){ }

	GL_Dots(const GL_Dots &m) = delete;
	GL_Dots &operator=(const GL_Dots &) = delete;

	void resize(size_t n_points);
	
	void draw() const;
	void depth_sort(const P3f &view);
	
	P3f       *points()       { return p.get(); }
	const P3f *points () const{ return p.get(); }
	
private:
	std::unique_ptr<P3f[]> p;
	size_t n_points;
};

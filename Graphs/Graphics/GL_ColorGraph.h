#pragma once
#include "GL_Graph.h"
#include "../Plot.h"
#include "../Geometry/Vector.h"
#include "../OpenGL/GL_Image.h"

class GL_ColorGraph : public GL_Graph
{
public:
	GL_ColorGraph(Graph &graph) : GL_Graph(graph){ }
	
	virtual void update(int n_threads, double quality);
	virtual void draw(GL_RM &rm) const;
	
	virtual bool needs_depth_sort() const{ return false; }
	virtual void depth_sort(const P3f &v){ (void)v; }

	virtual Opacity opacity() const;

	virtual bool has_unit_normals() const{ return true; } // has no normals

private:
	GL_Image im;
	float xr, yr, zr;
};

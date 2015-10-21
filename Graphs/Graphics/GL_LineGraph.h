#pragma once

#include "../OpenGL/GL_Dots.h"
#include "../OpenGL/GL_Lines.h"
#include "GL_Graph.h"

class GL_LineGraph : public GL_Graph
{
public:
	GL_LineGraph(Graph &graph) : GL_Graph(graph){ }
	
	virtual void update(int n_threads, double quality);
	virtual void draw(GL_RM &rm) const;

	virtual Opacity opacity() const;
	
	virtual bool needs_depth_sort() const;

	virtual void depth_sort(const P3f &view_vector)
	{
		if (!needs_depth_sort()) return;
		lines.depth_sort(view_vector);
		dots.depth_sort(view_vector);
	}
	
	virtual bool has_unit_normals() const{ return true; } // has no normals

protected:
	GL_Lines lines;
	GL_Dots  dots;
};

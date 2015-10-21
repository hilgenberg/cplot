#pragma once

#include "../OpenGL/GL_Dots.h"
#include "../OpenGL/GL_Lines.h"
#include "GL_LineGraph.h"

class GL_ImplicitLineGraph : public GL_LineGraph
{
public:
	GL_ImplicitLineGraph(Graph &graph) : GL_LineGraph(graph){ }
	
	virtual bool has_unit_normals() const{ return true; } // has no normals

	virtual void update(int n_threads, double quality);
};

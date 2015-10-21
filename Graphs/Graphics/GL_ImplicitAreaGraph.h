#pragma once
#include "GL_AreaGraph.h"

class GL_ImplicitAreaGraph : public GL_AreaGraph
{
public:
	GL_ImplicitAreaGraph(Graph &graph) : GL_AreaGraph(graph){ }
	
	virtual bool has_unit_normals() const{ return false; }

	virtual void update(int n_threads, double quality);
};

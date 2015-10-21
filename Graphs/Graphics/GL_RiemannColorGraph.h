#pragma once
#include "GL_AreaGraph.h"

class GL_RiemannColorGraph : public GL_AreaGraph
{
public:
	GL_RiemannColorGraph(Graph &graph) : GL_AreaGraph(graph){ }
	
	virtual void update(int n_threads, double quality);

	virtual bool has_unit_normals() const{ return true; }

private:
	void update(int n_threads, std::vector<void *> &info);
};

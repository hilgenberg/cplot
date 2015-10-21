#pragma once
#include "GL_PointGraph.h"

//----------------------------------------------------------------------------------------------------------------------
// GL_RiemannHistogramPointGraph
//----------------------------------------------------------------------------------------------------------------------

class GL_RiemannHistogramPointGraph : public GL_PointGraph
{
public:
	GL_RiemannHistogramPointGraph(Graph &graph) : GL_PointGraph(graph){ }
	
	virtual bool has_unit_normals() const{ return true; }

	virtual void update(int n_threads, double quality);
};

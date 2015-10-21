#pragma once
#include "GL_PointGraph.h"

//----------------------------------------------------------------------------------------------------------------------
// GL_HistogramPointGraph
//----------------------------------------------------------------------------------------------------------------------

class GL_HistogramPointGraph : public GL_PointGraph
{
public:
	GL_HistogramPointGraph(Graph &graph) : GL_PointGraph(graph){ }
	
	virtual bool has_unit_normals() const{ return true; }
	
	virtual void update(int n_threads, double quality);
};

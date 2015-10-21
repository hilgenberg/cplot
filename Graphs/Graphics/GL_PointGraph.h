#pragma once
#include "GL_Graph.h"
#include "../Geometry/Vector.h"
#include <memory>

//----------------------------------------------------------------------------------------------------------------------
// GL_PointGraph
//----------------------------------------------------------------------------------------------------------------------

class GL_PointGraph : public GL_Graph
{
public:
	GL_PointGraph(Graph &graph) : GL_Graph(graph), nvertexes(0){}
	
	virtual void update(int n_threads, double quality);
	virtual void draw(GL_RM &rm) const;

	virtual Opacity opacity() const;

	virtual void depth_sort(const P3f &view_vector);
	virtual bool needs_depth_sort() const;

	virtual bool has_unit_normals() const{ return true; } // has no normals

protected:
	std::unique_ptr <P3f []> pau; // base points
	std::unique_ptr <P3f []> vau; // vectors (for vector fields)
	size_t nvertexes;
	
private:
	float  max_len;  // for scaling vector fields (as is gridsize)
	float  gridsize; // distance between neighbouring grid points in any direction and in GL coordinates
};

#pragma once
#include "GL_Graph.h"
#include "../OpenGL/GL_Mesh.h"
#include "../OpenGL/GL_Mask.h"
#include "../Graph.h"

class GL_AreaGraph : public GL_Graph
{
public:
	GL_AreaGraph(Graph &graph) : GL_Graph(graph){ }
	
	virtual void update(int n_threads, double quality);
	virtual void draw(GL_RM &rm) const;

	virtual Opacity opacity() const;
	
	virtual bool needs_depth_sort() const;

	virtual void depth_sort(const P3f &view_vector)
	{
		if (!needs_depth_sort()) return;
		mesh.depth_sort(view_vector);
	}

	virtual bool has_unit_normals() const{ return graph.options.shading_mode == Shading_Flat; }

protected:
	GL_Mesh      mesh;
	GL_MaskScale mask_scale;
	
private:
	void update_without_subdivision(int n_threads, std::vector<void *> &info);
	//void update_with_subdivision(int n_threads, int depth, std::vector<void *> &info);
};

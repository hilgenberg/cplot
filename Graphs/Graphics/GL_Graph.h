#pragma once
#include "../Geometry/Vector.h"
class Graph;
struct Plot;
class GL_RM;

enum Opacity
{
	OPAQUE      = 0, // alpha is always 0 or 255
	ANTIALIASED = 1, // opaque if aa != AA_LINES, otherwise transparent
	TRANSPARENT = 2  // has transparency
};

class GL_Graph
{
public:
	GL_Graph(Graph &graph) : graph(graph){ }
	virtual ~GL_Graph(){ }

	virtual void draw(GL_RM &rm) const = 0;
	virtual void update(int n_threads, double quality) = 0;
	virtual bool needs_depth_sort() const = 0;
	virtual void depth_sort(const P3f &view_vector) = 0; // should use needs_depth_sort!
	
	virtual Opacity opacity() const = 0;
	virtual bool has_unit_normals() const = 0;
	virtual bool wants_backface_culling() const{ return false; }
	
protected:
	Graph &graph;
	
	void  start_drawing() const; // handles clipping
	void finish_drawing() const;
};

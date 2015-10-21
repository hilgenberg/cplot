#pragma once

#include "../Geometry/Vector.h"

#include <GL/gl.h>
#include <memory>
#include <vector>

class GL_Mesh
{
public:
	enum class NormalMode
	{
		Vertex,
		Face,
		None
	};

	GL_Mesh() : n_points(0), n_faces(0), n_normals(0), n_gridlines(0), max_index(0)
	{ }
	
	GL_Mesh(const GL_Mesh &m) = delete;
	GL_Mesh &operator=(const GL_Mesh &) = delete;

	void resize(size_t n_points, size_t n_faces, NormalMode n, bool textured);
	void clear();
	
	void draw(bool normals_are_unit) const; //!< caller must set up the texture arrays if needed!
	void draw_grid(bool full=false) const;
	void draw_normals() const; // for debugging
	
	void depth_sort(const P3f &view);
	
	GLuint *faces  (){ return f.get(); }
	P3f    *points (){ return p.get(); }
	P3f    *normals(){ return n.get(); }
	P2f    *texture(){ return t.get(); }
	GLuint *edges  (){ return e.get(); } //!< Two indexes into points per edge. @see close_gaps

	const GLuint *faces  () const{ return f.get(); }
	const P3f    *points () const{ return p.get(); }
	const P3f    *normals() const{ return n.get(); }
	const P2f    *texture() const{ return t.get(); }
	const GLuint *edges  () const{ return e.get(); }

	void set_grid(bool *edge_flags, bool remove_duplicates = true);
	void close_gaps(size_t total_per_chunk, const std::vector<size_t> &skipped, bool *edge_flags);
	
private:
	std::unique_ptr<P3f   []> p; //!< vertex coordinates
	std::unique_ptr<P3f   []> n; //!< normals; NULL, one per point, or one per face, depending on NormalMode
	std::unique_ptr<P2f   []> t; //!< texture coordinates, one per point
	std::unique_ptr<GLuint[]> f; //!< faces, n_faces*3 indexes into the other arrays
	std::unique_ptr<GLuint[]> e; //!< edges

	size_t n_points, n_faces, n_normals, n_gridlines;
	size_t max_index; // in faces/grid index array
	NormalMode nmode;
	
	void gen_vertex_normals(); // turn face- into vertex-normals
	void gen_normals(bool per_vertex = true, int n_threads = -1); // -1 = n_cores
};

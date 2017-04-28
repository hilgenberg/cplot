#include "GL_Mesh.h"
#include "GL_Util.h"

#include <set>
#include <algorithm>

void GL_Mesh::resize(size_t np, size_t nf, NormalMode nm, bool textured)
{
	nmode = nm;
	max_index = 0;
	e.reset(nullptr);
	n_gridlines = 0;
	
	if (n_points != np)
	{
		n_points = 0;
		p.reset(new P3f[np]);
		t.reset(textured ? new P2f[np] : nullptr);
		n_points = np;
	}
	else if (!textured)
	{
		t.reset(nullptr);
	}
	else if (textured && !t)
	{
		t.reset(new P2f[np]);
	}
	
	if (n_faces != nf)
	{
		n_faces = 0;
		f.reset(new GLuint[3*nf]);
		n_faces = nf;
	}

	size_t nn = (nm == NormalMode::Face ? nf : nm == NormalMode::Vertex ? np : 0);
	if (n_normals != nn)
	{
		n_normals = 0;
		n.reset(nn ? new P3f[nn] : nullptr);
		n_normals = nn;
	}
	if (nm == NormalMode::Vertex && nn > 0)
	{
		memset(n.get(), 0, nn*sizeof(P3f));
	}
}
void GL_Mesh::clear()
{
	n_points = n_faces = n_normals = n_gridlines = max_index = 0;
	nmode = NormalMode::None;
	p.reset(nullptr);
	n.reset(nullptr);
	t.reset(nullptr);
	f.reset(nullptr);
	e.reset(nullptr);
}

// edge_flags must have same size as faces and only be called after faces are set
void GL_Mesh::set_grid(bool *edges, bool remove_duplicates)
{
	if (!edges || !n_faces)
	{
		e.reset(nullptr);
		n_gridlines = 0;
		return;
	}
	GLuint *fa = faces(); assert(fa);

	if (remove_duplicates)
	{
		std::set<std::pair<GLuint, GLuint>> lines;
		for (size_t i = 0; i < 3*n_faces; ++i)
		{
			if (!edges[i]) continue;
			GLuint a = fa[i];
			GLuint b = fa[i + 1 - (i%3 == 2)*3];
			assert(a != b);
			if (a > b) std::swap(a, b);
			lines.insert(std::make_pair(a, b));
		}
		e.reset(new GLuint[2*lines.size()]);
		n_gridlines = lines.size();
		GLuint *ea = e.get();
		for (auto &i : lines)
		{
			*ea++ = i.first;
			*ea++ = i.second;
		}
	}
	else
	{
		size_t ne = 0;
		for (size_t i = 0; i < 3*n_faces; ++i) if (edges[i]) ++ne;
		
		e.reset(new GLuint[2*ne]);
		n_gridlines = ne;
		GLuint *ea = e.get();
		if (ne) for (size_t i = 0; i < 3*n_faces; ++i)
		{
			if (!edges[i]) continue;
			*ea++ = fa[i];
			*ea++ = fa[i + 1 - (i%3 == 2)*3];
			assert(ea[-1] != ea[-2]);
		}
	}
}

void GL_Mesh::close_gaps(size_t total_per_chunk, const std::vector<size_t> &skipped_faces, bool *edges)
{
	size_t nf0 = n_faces;
	GLuint *f0 = faces(), *fsrc = f0, *fdst = f0;
	P3f    *n0 = (nmode == NormalMode::Face ? normals() : NULL), *nsrc = n0, *ndst = n0;
	bool   *e0 = edges, *esrc = edges, *edst = edges;
	
	for (size_t skip : skipped_faces)
	{
		size_t nf = total_per_chunk;
		if (fsrc + 3*nf > f0 + 3*nf0) nf = (f0 + 3*nf0 - fsrc) / 3;
		n_faces -= skip;
		if (fsrc != fdst)
		{
			memmove(fdst, fsrc, 3*(nf-skip)*sizeof(GLuint));
			if (n0) memmove(ndst, nsrc, (nf-skip)*sizeof(P3f));
			if (e0) memmove(edst, esrc, 3*(nf-skip)*sizeof(bool));
		}
		fsrc += 3*nf;
		fdst += 3*(nf-skip);
		nsrc += nf;
		ndst += (nf-skip);
		esrc += 3*nf;
		edst += 3*(nf-skip);
	}
	
	#ifdef DEBUG
	for (size_t i = 0; i < 3*n_faces; ++i)
	{
		GLuint idx = f0[i];
		assert(idx < n_points);
	}
	#endif
}

void GL_Mesh::draw(bool unit_normals) const
{
	if (!n_points) return;
	
	const P3f    *na = normals();
	const GLuint *fa = faces();
	
	//------------------------------------------------------------------------------------------------------------------
	// draw triangles
	//------------------------------------------------------------------------------------------------------------------
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, points());
	
	if (n_normals && nmode == NormalMode::Face)
	{
		if (!unit_normals) glEnable(GL_NORMALIZE);
		glBegin(GL_TRIANGLES);
		for (size_t i = 0; i < n_faces; ++i)
		{
			glNormal3fv(na[i]);
			glArrayElement(fa[3*i+1]);
			glArrayElement(fa[3*i+2]);
			glArrayElement(fa[3*i  ]);
		}
		glEnd();
		glDisable(GL_NORMALIZE);
	}
	else if (n_normals)
	{
		if (!unit_normals) glEnable(GL_NORMALIZE);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, na);
		glDrawRangeElements(GL_TRIANGLES, 0,(GLuint)n_points, 3*(GLuint)n_faces, GL_UNSIGNED_INT, fa);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisable(GL_NORMALIZE);
	}
	else
	{
		glDrawRangeElements(GL_TRIANGLES, 0,(GLuint)n_points, 3*(GLuint)n_faces, GL_UNSIGNED_INT, fa);
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
}

void GL_Mesh::draw_grid(bool full) const
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, points());
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (full)
	{
		glEdgeFlag(GL_TRUE);
		glDrawRangeElements(GL_TRIANGLES, 0,(GLuint)n_points, 3*(GLuint)n_faces, GL_UNSIGNED_INT, faces());
		glEdgeFlag(GL_FALSE);
	}
	else
	{
		glDrawRangeElements(GL_LINES, 0,(GLuint)n_points, 2*(GLuint)n_gridlines, GL_UNSIGNED_INT, edges());
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void GL_Mesh::draw_normals() const
{
	const P3f    *va = points();
	const P3f    *na = normals();
	const GLuint *fa = faces();
	
	if (nmode == NormalMode::Face)
	{
		for (size_t i = 0; i < n_faces; ++i)
		{
			// draw normal on center of the face
			P3f p = va[fa[3*i  ]];
			p    += va[fa[3*i+1]];
			p    += va[fa[3*i+2]];
			p    /= 3.0f;
			
			P3f n = na[i];
			n.to_unit(); n *= 0.05f;
			draw_arrow3d(p, n, 0.01f);
		}
	}
	else
	{
		std::set<GLuint> done;
		for (size_t i = 0; i < 3*n_faces; ++i)
		{
			if (done.count(fa[i])) continue;
			done.insert(fa[i]);
			P3f n = na[fa[i]];
			n.to_unit(); n *= 0.05f;
			draw_arrow3d(va[fa[i]], n, 0.01f);
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Depth sorting
//----------------------------------------------------------------------------------------------------------------------

struct Face{ GLuint A,B,C; };
struct Edge{ GLuint A,B; };

struct Iter // for sorting faces and normals simultaneously
{
	typedef ptrdiff_t           difference_type;
	typedef std::pair<Face,P3f> value_type; // for temp copies in the STL code
	typedef value_type          Val;
	typedef std::random_access_iterator_tag iterator_category;

	struct Ref
	{
		inline Ref(const Ref &r) : face(r.face), normal(r.normal){}
		inline Ref(Face &f, P3f &n) : face(f), normal(n){}
		
		inline void operator=(const Ref &r){ face = r.face;  normal = r.normal; }
		inline void operator=(const Val &r){ face = r.first; normal = r.second; }
		
		inline operator value_type() const{ return std::make_pair(face, normal); }
		
		Face &face;
		P3f  &normal;
	};
	
	Iter(Face *f, P3f *n) : i1(f), i2(n){}
	inline Ref operator*() const{ return Ref(*i1, *i2); }
	inline difference_type operator-(const Iter &I) const{ return i1-I.i1; }
	inline Iter &operator++() { ++i1; ++i2; return *this; }
	inline Iter operator++(int) { Iter ret(*this); ++i1; ++i2; return ret; }
	inline Iter &operator--() { --i1; --i2; return *this; }
	inline Iter &operator+=(difference_type d) { i1 += d; i2 += d; return *this; }
	inline Iter &operator-=(difference_type d) { i1 -= d; i2 -= d; return *this; }
	inline Iter operator+(difference_type d) const { return Iter{i1+d, i2+d}; }
	inline Iter operator-(difference_type d) const { return Iter{i1-d, i2-d}; }
	inline bool operator==(const Iter &I) const{ return i1 == I.i1; }
	inline bool operator!=(const Iter &I) const{ return i1 != I.i1; }
	inline bool operator>=(const Iter &I) const{ return i1 >= I.i1; }
	inline bool operator<=(const Iter &I) const{ return i1 <= I.i1; }
	inline bool operator> (const Iter &I) const{ return i1 >  I.i1; }
	inline bool operator< (const Iter &I) const{ return i1 <  I.i1; }

	Face *i1;
	P3f  *i2;
};
namespace std{ template<> struct iterator_traits<Iter>
{
	typedef Iter::difference_type difference_type;
	typedef Iter::value_type      value_type;
	typedef Iter::Ref             reference;
	typedef void                  pointer; // not sure what this would have to be
	typedef std::random_access_iterator_tag iterator_category;
};}
inline void swap(const Iter::Ref &a, const Iter::Ref &b)
{
	std::swap(a.face,   b.face);
	std::swap(a.normal, b.normal);
}

struct FaceCmp
{
	P3f * const p;
	P3f   const view;
	
	inline bool operator() (const Face &f, const Face &g){ return p[f.A]*view > p[g.A]*view; }
	inline bool operator() (const Edge &a, const Edge &b){ return p[a.A]*view > p[b.A]*view; }
	
	inline bool operator() (const Iter::Val &a, const Iter::Val &b){ return p[a.first.A]*view > p[b.first.A]*view; }
	inline bool operator() (const Iter::Ref &a, const Iter::Ref &b){ return p[a.face. A]*view > p[b.face. A]*view; }
	inline bool operator() (const Iter::Ref &a, const Iter::Val &b){ return p[a.face. A]*view > p[b.first.A]*view; }
	inline bool operator() (const Iter::Val &a, const Iter::Ref &b){ return p[a.first.A]*view > p[b.face. A]*view; }
};

void GL_Mesh::depth_sort(const P3f &view)
{
	if (n_faces)
	{
		Face *fs = (Face*)faces();
		if (nmode != NormalMode::Face)
		{
			std::sort(fs, fs + n_faces, FaceCmp{points(),view});
		}
		else
		{
			P3f *ns = normals(); // normals must be sorted with the faces.
			Iter begin(fs,ns), end(fs + n_faces, ns + n_faces);
			FaceCmp cmp{ points(),view };
			std::sort(begin, end, cmp);
			//std::sort(Iter{fs,ns}, Iter{fs + n_faces, ns + n_faces}, FaceCmp{points(),view});
		}
	}
	
	if (n_gridlines)
	{
		Edge *es = (Edge*)edges();
		std::sort(es, es + n_gridlines, FaceCmp{points(),view});
	}
}


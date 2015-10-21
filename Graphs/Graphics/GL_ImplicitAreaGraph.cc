#include "GL_ImplicitAreaGraph.h"
#include "ThreadStorage.h"
#include "Info.h"
#include "../Geometry/Vector.h"
#include "../Threading/ThreadMap.h"
#include <GL/gl.h>
#include <vector>
#include <iostream>

extern int mc_edges[256];
extern int mc_tri[256][16];

//----------------------------------------------------------------------------------------------------------------------
// Data/Mesh storage
//----------------------------------------------------------------------------------------------------------------------
// 6-element arrays are ordered (xmin, xmax, ymin, ymax, zmin, zmax)
// 8-element arrays are ordered (xmin,ymin,zmin), (xmax, ymin, zmin), (xmin, ymax, zmin), (xmax, ymax, zmin), ... , (xmax, ymax, zmax)
//----------------------------------------------------------------------------------------------------------------------

struct Vertex
{
	POOL_ITEM(Vertex);
	
	P3f p;
	P3f normal;
	unsigned gl_index;
	char padding[32-7*4];
	static const unsigned npos = 0xFFFFFFFF;
	
	Vertex(double x, double y, double z, ThreadInfo &ti) : gl_index(npos)
	{
		assert(defined(x) && defined(y) && defined(z));
		normal.clear();
		ti.ia.map(P3d(x,y,z), p);
		assert(p.x >= -1.0f && p.x <= 1.0f);
	}
};
struct Face
{
	POOL_ITEM(Face);
	
	Vertex *v[3];
	bool    e[3]; // edge flags
	char padding[32-3*8-3];
	
	Face(Vertex *a, Vertex *b, Vertex *c, ThreadInfo &ti)
	{
		assert(a && b && c);
		v[0] = a;
		v[1] = b;
		v[2] = c;
		(void)ti;
		#ifndef DEBUG
		if (ti.ic.vertex_normals)
		#endif
		{
			P3f d1, d2, n;
			sub(d1, b->p, a->p);
			sub(d2, c->p, a->p);
			cross(n, d1, d2);
			n.to_unit();
			
			if (a->normal * n < 0.0f) a->normal -= n; else a->normal += n;
			if (b->normal * n < 0.0f) b->normal -= n; else b->normal += n;
			if (c->normal * n < 0.0f) c->normal -= n; else c->normal += n;
			/*a->normal += n;
			b->normal += n;
			c->normal += n;*/
		}
	}
	
};

struct Cube
{
	POOL_ITEM(Cube);
	
	// at most one vertex per edge
	union
	{
		struct
		{
			// ordered as in the marching cube code
			Vertex // v<xyz>, 0 = min, 1 = max, underscore = from min to max in that dimension
			*v_00, *v1_0, *v_10, *v0_0,
			*v_01, *v1_1, *v_11, *v0_1,
			*v00_, *v10_, *v11_, *v01_;
		};
		Vertex *v[12];
	};

	Cube(){ memset(v, 0, 12*sizeof(Vertex*)); }
};

//----------------------------------------------------------------------------------------------------------------------
// Recursive marching cubes
//----------------------------------------------------------------------------------------------------------------------

#define SET8(X, a, b, c, d, e, f, g, h) do{ X[0]=(a); X[1]=(b); X[2]=(c); X[3]=(d); X[4]=(e); X[5]=(f); X[6]=(g); X[7]=(h); }while(0)
#define SET6(X, a, b, c, d, e, f)       do{ X[0]=(a); X[1]=(b); X[2]=(c); X[3]=(d); X[4]=(e); X[5]=(f); }while(0)

#define X0 x[0]
#define X2 x[1]
#define Y0 x[2]
#define Y2 x[3]
#define Z0 x[4]
#define Z2 x[5]
inline char gmask(const bool g[6])
{
	char m = 0;
	if (g[0]) m |= 1;
	if (g[1]) m |= 2;
	if (g[2]) m |= 4;
	if (g[3]) m |= 8;
	if (g[4]) m |= 16;
	if (g[5]) m |= 32;
	return m;
}
#define E(i, a,b,c,d) if (edge==a || edge==b || edge==c || edge==d) m |= i
inline char gmask(int edge)
{
	char m = 0;
	E(1, 3,7,8,11);
	E(2, 1,5,9,10);
	E(4, 0,4,8,9);
	E(8, 2,6,10,11);
	if (edge < 4) m |= 16;
	if (edge >= 4 && edge < 8) m |= 32;
	return m;
}

#define BISECT 3
#define MID(x0,x1, f0,f1) (fabs(f0-f1)>1e-12 ? ((x1*f0 - x0*f1) / (f0 - f1)) : 0.5*(x0+x1))
inline double midx(double x1, double x2, double y, double z, double f1, double f2, ThreadInfo &ti)
{
	if (!defined(f1) || !defined(f2)) return 0.5*(x1+x2);
	assert(f1*f2 <= 0.0);
	for (int i = 0; i < BISECT; ++i)
	{
		double m = 0.5*(x1+x2), f = ti.eval(m, y, z, false, i > 0, i > 0);
		if (!defined(f)) return m;
		if (f1*f > 0.0){ f1 = f; x1 = m; }else{ f2 = f; x2 = m; }
	}
	return MID(x1,x2, f1,f2);
}
inline double midy(double x, double y1, double y2, double z, double f1, double f2, ThreadInfo &ti)
{
	if (!defined(f1) || !defined(f2)) return 0.5*(y1+y2);
	assert(f1*f2 <= 0.0);
	for (int i = 0; i < BISECT; ++i)
	{
		double m = 0.5*(y1+y2), f = ti.eval(x, m, z, i > 0, false, i > 0);
		if (!defined(f)) return m;
		if (f1*f > 0.0){ f1 = f; y1 = m; }else{ f2 = f; y2 = m; }
	}
	return MID(y1,y2, f1,f2);
}
inline double midz(double x, double y, double z1, double z2, double f1, double f2, ThreadInfo &ti)
{
	if (!defined(f1) || !defined(f2)) return 0.5*(z1+z2);
	assert(f1*f2 <= 0.0);
	for (int i = 0; i < BISECT; ++i)
	{
		double m = 0.5*(z1+z2), f = ti.eval(x, y, m, i > 0, i > 0, false);
		if (!defined(f)) return m;
		if (f1*f > 0.0){ f1 = f; z1 = m; }else{ f2 = f; z2 = m; }
	}
	return MID(z1,z2, f1,f2);
}
#undef BISECT

static Cube *march(Cube *neighbour[6], const double x[6], const double f[8], const bool grid[6], ThreadInfo &ti, ThreadStorage<Face> &ts)
{
	/*#ifdef DEBUG
	#define assertf(x,y) do{ double tmp = y; assert(!defined(x) && !defined(tmp) || fabs(x-tmp) < 1e-12); }while(0)
	assertf(f[0], ti.eval(X0, Y0, Z0));
	assertf(f[1], ti.eval(X2, Y0, Z0));
	assertf(f[2], ti.eval(X0, Y2, Z0));
	assertf(f[3], ti.eval(X2, Y2, Z0));
	assertf(f[4], ti.eval(X0, Y0, Z2));
	assertf(f[5], ti.eval(X2, Y0, Z2));
	assertf(f[6], ti.eval(X0, Y2, Z2));
	assertf(f[7], ti.eval(X2, Y2, Z2));
	#undef assertf
	#endif*/
	
	int mask = 0;

	if (!defined(f[0])) return NULL; if (f[0] < 0.0) mask |= 1;
	if (!defined(f[1])) return NULL; if (f[1] < 0.0) mask |= 2;
	if (!defined(f[3])) return NULL; if (f[3] < 0.0) mask |= 4;
	if (!defined(f[2])) return NULL; if (f[2] < 0.0) mask |= 8;
	if (!defined(f[4])) return NULL; if (f[4] < 0.0) mask |= 16;
	if (!defined(f[5])) return NULL; if (f[5] < 0.0) mask |= 32;
	if (!defined(f[7])) return NULL; if (f[7] < 0.0) mask |= 64;
	if (!defined(f[6])) return NULL; if (f[6] < 0.0) mask |= 128;
	if (mask == 255 || mask == 0) return NULL;

	if (ti.is.detect_discontinuities)
	{
		/*double fm = ti.eval(0.5*(X0+X2), 0.5*(Y0+Y2), 0.5*(Z0+Z2));
		if (!defined(fm)) return NULL;
		if (fabs(fm) > 0.1) return NULL;*/
		
		#define APPLY8(F) F(F( F(PRE(f[0]),PRE(f[1])), F(PRE(f[2]),PRE(f[3])) ), F( F(PRE(f[4]),PRE(f[5])), F(PRE(f[6]),PRE(f[7])) ))
		#define PRE(x) ((x)<0.0 ? 10000.0 : (x))
		double min = APPLY8(std::min);
		#undef PRE
		#define PRE(x) ((x)>0.0 ? -10000.0 : x)
		double max = APPLY8(std::max);
		#undef PRE

		if (min - max > 0.1) return NULL;
		
		//double a = fabs(min-fm), b = fabs(max-fm);
		//if (a > 10.0 * b || b > 10.0 * a) return NULL;
		
		/*double min = std::min(std::min(std::min(abs(f[0]-fm), abs(f[1]-fm)), std::min(abs(f[2]-fm), abs(f[3]-fm))),
							  std::min(std::min(abs(f[4]-fm), abs(f[5]-fm)), std::min(abs(f[6]-fm), abs(f[7]-fm))));
		double max = std::max(std::max(std::max(abs(f[0]-fm), abs(f[1]-fm)), std::max(abs(f[2]-fm), abs(f[3]-fm))),
							  std::max(std::max(abs(f[4]-fm), abs(f[5]-fm)), std::max(abs(f[6]-fm), abs(f[7]-fm))));
		if (max > 20.0*min) return NULL;*/
	}

	
	Cube *c = new (ts.pool) Cube;
	char gm = gmask(grid);
	
	// edges are shared by four cubes, this one, two of its neighbours and their other intersecting neighbour
	// we don't test the last one, which might somwhat screw up non-deterministic plots...
	// this could be be a problem with certain diagonal moves, which we don't do (I think)
	#define NN(a1,b1, a2,b2, x,y,z) (\
	(neighbour[a1] && neighbour[a1]->v[b1]) ? neighbour[a1]->v[b1] : \
	(neighbour[a2] && neighbour[a2]->v[b2]) ? neighbour[a2]->v[b2] : \
	(++ts.vertex_count, new(ts.pool) Vertex(x,y,z, ti)))

	#define assertf(x,y) do{ assert(defined(x) && defined(y) && fabsf((x)-(y)) < 1e-8f); }while(0)
	#ifdef DEBUG
	#define SETV(i, a1,b1, a2,b2, X,Y,Z) do{\
		c->v[i] = NN(a1,b1, a2,b2, X,Y,Z);\
		P3f p;\
		ti.ia.map(P3d(X,Y,Z), p);\
		assertf(c->v[i]->p.x, p.x);\
		assertf(c->v[i]->p.y, p.y);\
		assertf(c->v[i]->p.z, p.z);\
	}while(0)
	#else
	#define SETV(i, a1,b1, a2,b2, x,y,z) c->v[i] = NN(a1,b1, a2,b2, x,y,z)
	#endif

	int edges = mc_edges[mask];
	if ((edges &    1)) SETV(0, 2,2, 4,4, midx(X0,X2, Y0, Z0, f[0],f[1], ti), Y0, Z0);
	if ((edges &    2)) SETV(1, 1,3, 4,5, X2, midy(X2, Y0,Y2, Z0, f[1],f[3], ti), Z0);
	if ((edges &    4)) SETV(2, 3,0, 4,6, midx(X0,X2, Y2, Z0, f[2],f[3], ti), Y2, Z0);
	if ((edges &    8)) SETV(3, 0,1, 4,7, X0, midy(X0, Y0,Y2, Z0, f[0],f[2], ti), Z0);
	
	if ((edges &   16)) SETV(4, 2,6, 5,0, midx(X0,X2, Y0, Z2, f[4],f[5], ti), Y0, Z2);
	if ((edges &   32)) SETV(5, 1,7, 5,1, X2, midy(X2, Y0,Y2, Z2, f[5],f[7], ti), Z2);
	if ((edges &   64)) SETV(6, 3,4, 5,2, midx(X0,X2, Y2, Z2, f[6],f[7], ti), Y2, Z2);
	if ((edges &  128)) SETV(7, 0,5, 5,3, X0, midy(X0, Y0,Y2, Z2, f[4],f[6], ti), Z2);
	
	if ((edges &  256)) SETV(8,  0,9, 2,11, X0, Y0, midz(X0, Y0, Z0,Z2,f[0],f[4], ti));
	if ((edges &  512)) SETV(9,  1,8, 2,10, X2, Y0, midz(X2, Y0, Z0,Z2,f[1],f[5], ti));
	if ((edges & 1024)) SETV(10, 1,11, 3,9, X2, Y2, midz(X2, Y2, Z0,Z2,f[3],f[7], ti));
	if ((edges & 2048)) SETV(11, 0,10, 3,8, X0, Y2, midz(X0, Y2, Z0,Z2,f[2],f[6], ti));

	#undef NN
	#undef MID

	#ifdef DEBUG
	P3f p0, p2;
	ti.ia.map(P3d(X0,Y0,Z0), p0);
	ti.ia.map(P3d(X2,Y2,Z2), p2);

	if ((edges &    1)){ assertf(c->v[0]->p.y, p0.y); assertf(c->v[0]->p.z, p0.z); }
	if ((edges &    2)){ assertf(c->v[1]->p.x, p2.x); assertf(c->v[1]->p.z, p0.z); }
	if ((edges &    4)){ assertf(c->v[2]->p.y, p2.y); assertf(c->v[2]->p.z, p0.z); }
	if ((edges &    8)){ assertf(c->v[3]->p.x, p0.x); assertf(c->v[3]->p.z, p0.z); }
	
	if ((edges &   16)){ assertf(c->v[4]->p.y, p0.y); assertf(c->v[4]->p.z, p2.z); }
	if ((edges &   32)){ assertf(c->v[5]->p.x, p2.x); assertf(c->v[5]->p.z, p2.z); }
	if ((edges &   64)){ assertf(c->v[6]->p.y, p2.y); assertf(c->v[6]->p.z, p2.z); }
	if ((edges &  128)){ assertf(c->v[7]->p.x, p0.x); assertf(c->v[7]->p.z, p2.z); }
	
	if ((edges &  256)){ assertf(c->v[ 8]->p.x, p0.x); assertf(c->v[ 8]->p.y, p0.y); }
	if ((edges &  512)){ assertf(c->v[ 9]->p.x, p2.x); assertf(c->v[ 9]->p.y, p0.y); }
	if ((edges & 1024)){ assertf(c->v[10]->p.x, p2.x); assertf(c->v[10]->p.y, p2.y); }
	if ((edges & 2048)){ assertf(c->v[11]->p.x, p0.x); assertf(c->v[11]->p.y, p2.y); }
	#endif
					   
	for (int *p = mc_tri[mask]; *p >= 0; p += 3)
	{
		Face *f = new(ts.faces) Face(c->v[p[0]], c->v[p[1]], c->v[p[2]], ti);
		++ts.face_count;
		char ma = gmask(p[0]);
		char mb = gmask(p[1]);
		char mc = gmask(p[2]);
		f->e[0] = (ma & mb & gm);
		f->e[1] = (mb & mc & gm);
		f->e[2] = (mc & ma & gm);
	}
	
	return c;
}
#undef X0
#undef X2
#undef Y0
#undef Y2
#undef Z0
#undef Z2

//----------------------------------------------------------------------------------------------------------------------
// Implicit area graph - worker threads
//----------------------------------------------------------------------------------------------------------------------

static void gridWorker(ThreadInfo &ti, int i1, int i2, ThreadStorage<Face> &ts,
					   std::vector<double> &f, std::vector<Cube*> &cells, bool between)
{
	// between means we are on the second run through and the outer rows are already done
	
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------
	
	const DI_Grid &ig = ti.ig;
	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	int nz = ig.z.nlines();
	
	bool disco = ti.is.detect_discontinuities;
	
	//------------------------------------------------------------------------------------------------------------------
	// build and refine the grid
	//------------------------------------------------------------------------------------------------------------------
	
	int iend = std::min(i2, nz-1);
	for (int i = i1 + (between ? 1 : 0); i <= iend; ++i)
	{
		double zi = ig.z[i], zii = ig.z[i > 0 ? i-1 : 0];
		
		for (int j = 0; j < ny; ++j)
		{
			double yj = ig.y[j], yjj = ig.y[j > 0 ? j-1 : 0];
			
			for (int k = 0; k < nx; ++k)
			{
				double xk = ig.x[k];
				
				int idx = nx*ny*i + nx*j + k; // vertex index
				
				if (!(between && i == i2))
				{
					f[idx] = ti.eval(xk, yj, zi, false, !disco && i == i1 && k > 0, !disco && i == i1 && k+j > 0);
				}
				
				if (i == i1 || j == 0 || k == 0) continue; // skip the cells until we have enough data
				
				double xkk = ig.x[k-1];

				//------------------------------------------------------------------------------------------------------
				// do the cell
				//------------------------------------------------------------------------------------------------------
				
				int cidx = (nx-1)*(ny-1)*(i-1) + (nx-1)*(j-1) + k-1; // cell index
				Cube  *nn[6];
				double xx[6], ff[8];
				bool   gg[6];
				SET6(nn, k>1 ? cells[cidx-1] : NULL, k < nx-1 ? cells[cidx+1] : NULL,
				         j>1 ? cells[cidx-nx+1] : NULL, j < ny-1 ? cells[cidx+nx-1] : NULL,
				         i>1 ? cells[cidx-(nx-1)*(ny-1)] : NULL, i < nz-1 ? cells[cidx+(nx-1)*(ny-1)] : NULL);
				SET8(ff, f[idx-nx*ny-nx-1], f[idx-nx*ny-nx], f[idx-nx*ny-1], f[idx-nx*ny], f[idx-nx-1], f[idx-nx], f[idx-1], f[idx]);
				SET6(gg, ig.x.visible(k-1), ig.x.visible(k), ig.y.visible(j-1), ig.y.visible(j), ig.z.visible(i-1), ig.z.visible(i));
				SET6(xx, xkk, xk, yjj, yj, zii, zi);
				cells[cidx] = march(nn, xx, ff, gg, ti, ts);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Counter/Allocator
//----------------------------------------------------------------------------------------------------------------------

static void collector(ThreadInfo &ti, std::vector<ThreadStorage<Face>> &tss, GL_Mesh &mesh, std::unique_ptr<bool[]> &eau)
{
	size_t nvertexes = 0;
	size_t nfaces    = 0;
	
	for (ThreadStorage<Face> &ts : tss)
	{
		ts.vertex_offset = nvertexes;
		ts.face_offset   = nfaces;
		
		nfaces    += ts.face_count;
		nvertexes += ts.vertex_count;
	}
	
	bool flat       = ti.ic.face_normals;
	bool do_normals = (flat || ti.ic.vertex_normals);
	bool texture    = ti.ic.texture;
	bool grid       = ti.ic.do_grid;
	
	GL_Mesh::NormalMode nm = !do_normals ? GL_Mesh::NormalMode::None :
	                         flat ? GL_Mesh::NormalMode::Face : GL_Mesh::NormalMode::Vertex;
	
	mesh.resize(nvertexes, nfaces, nm, texture);
	
	eau.reset(grid ? new bool[3*nfaces] : NULL);
}

//----------------------------------------------------------------------------------------------------------------------
// Transfer
//----------------------------------------------------------------------------------------------------------------------

static void transfer(ThreadInfo &ti, ThreadStorage<Face> &ts, GL_Mesh &mesh, std::unique_ptr<bool[]> &eau)
{
	P3f    *va = mesh.points();
	P3f    *na = mesh.normals();
	GLuint *fa = mesh.faces();
	bool   *ea = eau.get();
	P2f    *ta = mesh.texture();
	const bool flat           = ti.ic.face_normals;
	const bool vertex_normals = ti.ic.vertex_normals;
	const bool grid           = ti.ic.do_grid;
	const bool texture        = ti.ic.texture;
	
	if (va) va += ts.vertex_offset;
	if (na) na += (flat ? ts.face_offset : ts.vertex_offset);
	fa += 3*ts.face_offset;
	if (ea) ea += 3*ts.face_offset;
	if (ta) ta += ts.vertex_offset;
	unsigned iv = (unsigned)ts.vertex_offset;
	
	PoolIterator<Face> fi(ts.faces.begin());
	for (size_t i = 0; fi; ++i, ++fi)
	{
		assert(ts.face_count-- > 0);
		const Face &f = *fi;
		
		if (flat)
		{
			P3f d1, d2;
			sub(d1, f.v[1]->p, f.v[0]->p);
			sub(d2, f.v[2]->p, f.v[0]->p);
			cross(*na++, d1, d2);
		}
		for (int j = 0; j < 3; ++j)
		{
			Vertex &v = *f.v[j];
			if (v.gl_index >= Vertex::npos)
			{
				assert(ts.vertex_count-- > 0);
				v.gl_index = iv++;
				*va++ = v.p;
				if (vertex_normals) *na++ = v.normal;
				if (texture) (ta++)->set(0.5f*v.p.x+0.5f, 0.5f-0.5f*v.p.y);
			}
			*fa++ = v.gl_index;
		}
		
		if (grid)
		{
			*ea++ = f.e[0];
			*ea++ = f.e[1];
			*ea++ = f.e[2];
		}
	}
	
	assert(ts.face_count == 0);
	assert(ts.vertex_count == 0);
}


//----------------------------------------------------------------------------------------------------------------------
// Main method
//----------------------------------------------------------------------------------------------------------------------

void GL_ImplicitAreaGraph::update(int n_threads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	DI_Calc ic(graph);
	if (graph.type() != R3_R || !ic.e0 || ic.dim != 1) return;
	
	DI_Axis ia(graph, true, false);
	if (ia.pixel <= 0.0) return;

	bool hiddenline = (graph.options.shading_mode == Shading_Hiddenline);
	bool wireframe  = (graph.options.shading_mode == Shading_Wireframe);
	bool flatshade = (graph.options.shading_mode == Shading_Flat);
	bool do_normals = (!ia.is2D && !hiddenline && !wireframe);
	bool grid = (wireframe || graph.options.grid_style == Grid_On);
	bool texture = !wireframe; // this could be more precise but then the settingsBox pays for it...
	double q0 = graph.options.quality;
	
	ic.vertex_normals = (do_normals && !flatshade);
	ic.face_normals   = (do_normals &&  flatshade);
	ic.texture        = texture;
	ic.do_grid        = grid;
	
	DI_Subdivision is;
	is.detect_discontinuities = graph.options.disco;
	is.max_faces = (size_t)((quality*q0*1000.0 + 1.0)*1000.0);
	int ngrid = (int)ceil(cbrt(is.max_faces));
	if (ngrid < 12) ngrid = 12;
	DI_Grid ig(ia, graph.options.grid_density, ngrid, true);
	
	int nx = ig.x.nlines(), ny = ig.y.nlines(), nz = ig.z.nlines();
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&is);
	info.push_back(&ig);
	
	double xr = 2.0 * ia.in_range[0], yr = 2.0 * ia.in_range[1];
	mask_scale.set(ig.x.vis_delta() / xr,
				   ig.y.vis_delta() / yr,
				   (ig.x.first_vis() - ia.in_min[0]) / xr,
				   (ig.y.first_vis() - ia.in_min[1]) / yr);
	
	//------------------------------------------------------------------------------------------------------------------
	// (2) build and refine the mesh
	//------------------------------------------------------------------------------------------------------------------
	
	Task task(&info);
	WorkLayer *layer = new WorkLayer("grid", &task, NULL, 1);
	
	std::vector<Cube*> cells((nx-1)*(ny-1)*(nz-1), NULL);
	std::vector<double> fvals(nx*ny*nz);
	
	int chunk = (nz+4*n_threads-1) / (4*n_threads); // smaller chunks because the workload will vary very much
	if (chunk < 1) chunk = 1;
	
	std::vector<int> chunks; // starting rows
	bool between = false;
	for (int i = 0; ; between = !between)
	{
		chunks.push_back(i);
		i += chunk;
		
		if (i >= nz - (between ? 0 : 1))
		{
			chunks.push_back(i);
			break;
		}
	}
	
	std::vector<ThreadStorage<Face>> storage(chunks.size()-1);
	
	for (int i = 0; i < (int)chunks.size()-1; ++i)
	{
		layer->add_unit([&,i](void *ti)
		{
			gridWorker(*(ThreadInfo*)ti, chunks[i], chunks[i+1], storage[i], fvals, cells, i&1);
		});
	}

	//------------------------------------------------------------------------------------------------------------------
	// (3) count the vertexes and faces, allocate arrays for their GL counterparts
	//------------------------------------------------------------------------------------------------------------------
	
	std::unique_ptr<bool[]> eau;
	layer = new WorkLayer("counter", &task, layer, 0, -1);
	layer->add_unit([&](void *ti)
	{
		collector(*(ThreadInfo*)ti, storage, mesh, eau);
	});
	
	//------------------------------------------------------------------------------------------------------------------
	// (4) transfer the mesh to the GL arrays
	//------------------------------------------------------------------------------------------------------------------
	
	layer = new WorkLayer("transfer", &task, layer, 1, -1);

	for (int i = 0; i < (int)storage.size(); ++i)
	{
		layer->add_unit([&,i](void *ti)
		{
			transfer(*(ThreadInfo*)ti, storage[i], mesh, eau);
		});
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// (5) run the entire thing
	//------------------------------------------------------------------------------------------------------------------
	
	task.run(n_threads);
	
	mesh.set_grid(eau.get());
}


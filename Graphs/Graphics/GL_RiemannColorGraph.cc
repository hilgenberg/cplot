#include "GL_RiemannColorGraph.h"
#include "../OpenGL/GL_Mesh.h"
#include "../../Utility/MemoryPool.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "../OpenGL/GL_Util.h"
#include "GL_Graph.h"
#include "../Geometry/Vector.h"
#include "../OpenGL/GL_Image.h"

#include <vector>
#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// update worker: fill in one subdivided triangle
//----------------------------------------------------------------------------------------------------------------------

static void update(ThreadInfo &ti, int data_index, GL_Mesh &m, double th, const P3d &A, const P3d &B, const P3d &C, int k, TextureProjection tp, size_t &skipped_faces)
{
	#ifdef DEBUG
	double l1 = (A-B).abs(), l2 = (B-C).abs(), l3 = (C-A).abs();
	assert(fabs(l1-l2) < 1e-8 && fabs(l1-l3) < 1e-8);
	#endif
	
	const DI_Calc &ic = ti.ic;
	BoundContext  &ec = ti.ec;

	// k = number of subdivisions of each side
	assert(k >= 1);
	const size_t nv = (size_t)(k+1)*(k+2)/2;
	
	P3f    *v = m.points () + nv * data_index;
	P3f    *n = m.normals() + nv * data_index;
	P2f    *t = m.texture() + nv * data_index;
	
	std::unique_ptr<bool[]> def0(new bool[nv]);
	bool *def = def0.get();
	
	/* A = 0
	       | \
	       1--2
	       | \| \
	   B = 3--4--5 = C */
	
	P3f a = (P3f)(A/k), b = (P3f)(B/k), c = (P3f)(C/k);
	
	// (1) calculate the grid points

	for (int i = 0; i <= k; ++i) // top to bottom
	{
		for (int j = 0; j <= i; ++j, ++t, ++v, ++n) // left to right
		{
			//P3d P = A + ((double)i / k)*(B-A) + ((double)j / k)*(C-B);
			*v = a*float(k-i) + b*float(i-j) + c*float(j);
			v->to_unit();
			
			*n = *v;
			
			cnum z; riemann(*v, z);
			
			*v *= 0.999f;
			
			if (ic.xi >= 0) ec.set_input(ic.xi, z);
			ec.eval();
			z = ec.output(0);
			if ((*def++ = defined(z)))
			{
				switch (tp)
				{
					case TP_Repeat:
					{
						double x =  z.real();
						double y = -z.imag() / th;
						t->x = (float)x*2.0f;
						t->y = (float)y*2.0f;
						break;
					}
					
					default:
					case TP_Center:
					{
						double x =  z.real();
						double y = -z.imag() / th;
						if (abs(x) <= 1.0 && abs(y) <= 1.0)
						{
							t->x = (float)(0.5*x+0.5);
							t->y = (float)(0.5*y+0.5);
						}
						else
						{
							def[-1] = false; // TODO
						}
						break;
					}
						
					case TP_Riemann:
					{
						/***********************************************************************************************
						 (1) project z onto riemann sphere:
						 l = 2 / (|z|² + 1)
						 q.x = l * rez
						 q.y = l * imz
						 q.z = l - 1
						 
						 (2) find the (spherical) distance from the north pole to q, normalize to [0,1]
						 d = arccos(q.z) / π
						 
						 (3) find the texture coords on a unit disk
						 z *= d / |z|
						 
						 (4) project onto texture space [0,2] x [0, 2], flipping y:
						     w < h, th > 1: onto [0,2]x[1-1/th,1+1/th] by (1+x, 1-y/th)
						     w > h, th < 1: onto [1-th,1+th]x[0,2] by (1+x*th, 1-y)
						 **********************************************************************************************/
						double tr = M_1_PI * 0.5 * 0.99999;
						double fx = std::min(1.0, th), fy = std::min(1.0, 1.0/th);
						double r = abs(z);
						if (r > 0.0)
						{
							double f = tr * acos(2.0 / (r*r + 1.0) - 1.0) / r;
							t->x = (float)(0.5 + f*z.real()*fx);
							t->y = (float)(0.5 - f*z.imag()*fy);
						}
						else
						{
							t->x = 0.5f;
							t->y = 0.5f;
						}
						break;
					}
						
					case TP_UV:
					{
						/***********************************************************************************************
						 (1) project z onto riemann sphere:
						 l = 2 / (|z|² + 1)
						 q.x = l * rez
						 q.y = l * imz
						 q.z = l - 1;
						 
						 (2) find its spherical coordinates when N = 0, S = ∞
						 phi   = arccos(q.z)     in [ 0, π]
						 theta = arctan(q.y/q.x) in [-π, π]
						 
						 (3) map range to texture range
						 x = theta/2π
						 y = phi/π
						 **********************************************************************************************/
						
						double phi   = acos(2.0 / (absq(z) + 1.0) - 1.0) / M_PI;
						double theta = atan2(z.imag(), z.real()) / M_PI + 1.0;
						
						t->x = (float)(theta * 0.5);
						t->y = (float)(phi);
						break;
					}
				}
			}
		}
	}
	
	
	// (2) build the faces
	
	def = def0.get();
	GLuint *f = m.faces  () + (size_t)k*k * data_index * 3;
	skipped_faces = (size_t)k*k;
	
	/* 0           i = 0
	   | \
	   1--2        i = 1
	   | \| \
	   3--4--5     i = 2
	   | \| \| \
	   6--7--8--9  i = 3 */
	
	for (int i = 1; i <= k; ++i) // top to bottom
	{
		GLuint q0 = i*(i-1)/2; // index of first vertex on row i-1
		GLuint q1 = i*(i+1)/2; // index of first vertex on row i
		GLuint p0 = i*(i-1)/2 + (GLuint)(nv * data_index); // index of first vertex on row i-1
		GLuint p1 = i*(i+1)/2 + (GLuint)(nv * data_index); // index of first vertex on row i
		
		for (int j = 0; j < i; ++j) // left to right
		{
			if (def[q0+j] && def[q1+j+1])
			{
				if (def[q1+j])
				{
					*f++ = p0+j;
					*f++ = p1+j+1;
					*f++ = p1+j;
					--skipped_faces;
				}
				if (j < i-1 && def[q1+j+1])
				{
					*f++ = p0+j;
					*f++ = p0+j+1;
					*f++ = p1+j+1;
					--skipped_faces;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update
//----------------------------------------------------------------------------------------------------------------------

void GL_RiemannColorGraph::update(int nthreads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------

	assert(graph.type() == C_C);
	assert(graph.isColor());

	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim <= 0 || ic.dim != 1 || graph.options.texture.empty())
	{
		mesh.clear();
		return;
	}
	ic.vertex_normals = false; // same as vertex coordinates
	ic.face_normals   = false; // not supported
	ic.texture        = true;  // we do these ourselves
	ic.do_grid        = false; // no edge flags

	DI_Axis ia(graph, false, false); // unused
	DI_Grid        ig;
	DI_Subdivision is;
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&is);
	info.push_back(&ig);
	
	//------------------------------------------------------------------------------------------------------------------
	// (2) calculation
	//------------------------------------------------------------------------------------------------------------------

	#define P 1.61803398874989484820458683437 /* = φ = 1+√5 / 2 */
	static P3d V[12] =
	{
		{-1.0, 0.0,  P }, {1.0, 0.0,   P }, {-1.0,  0.0,  -P }, {1.0,  0.0,  -P },
		{ 0.0,  P , 1.0}, {0.0,  P , -1.0}, { 0.0,  -P ,  1.0}, {0.0,  -P , -1.0},
		{  P , 1.0, 0.0}, {-P , 1.0,  0.0}, {  P , -1.0,  0.0}, {-P , -1.0,  0.0}
	};
	#undef P
	static int F[20][3] =
	{
		{ 0, 4, 1}, { 0, 9, 4}, { 9, 5, 4}, { 4, 5, 8}, { 4, 8, 1},
		{ 8,10, 1}, { 8, 3,10}, { 5, 3, 8}, { 5, 2, 3}, { 2, 7, 3},
		{ 7,10, 3}, { 7, 6,10}, { 7,11, 6}, {11, 0, 6}, { 0, 1, 6},
		{ 6, 1,10}, { 9, 0,11}, { 9,11, 2}, { 9, 2, 5}, { 7, 2,11}
	};
	
	double q0 = graph.options.quality;
	const size_t max_faces = (size_t)(quality*q0*1e6)+20;
	// we start with an icosahedron that has 20 faces. Each edge can be subdivided into k pieces, which
	// gives k^2 subtriangles. So we want 20*k^2 <= max_faces.
	const int k = std::max(1, (int)floor(sqrt(max_faces/20.0)));
	
	// So we will have k^2 triangles on (k+1)(k+2)/2 vertices in each face.
	// For vertex normals we pack them tightly, for face normals we would need 3*k^2 vertices ((5k^2-3k-2)/2 more).
	
	size_t nvertexes = (size_t)(k+1)*(k+2)*10;
	size_t nfaces    = (size_t)k*k*20;
	mesh.resize(nvertexes, nfaces, GL_Mesh::NormalMode::Vertex, true);
	std::vector<size_t> skipped_faces(20, 0);
	
	double th = (double)graph.options.texture.h() / graph.options.texture.w();
	
	Task task(&info);
	WorkLayer *layer = new WorkLayer("calculate", &task, NULL);
	for (int i = 0; i < 20; ++i)
	{
		layer->add_unit([=,&skipped_faces](void *ti)
		{
			::update(*(ThreadInfo*)ti, i, mesh, th, V[F[i][0]], V[F[i][1]], V[F[i][2]],
					 k, graph.options.texture_projection, skipped_faces[i]);
		});
	}
	task.run(nthreads);
	
	mesh.close_gaps(nfaces/20, skipped_faces, NULL);
}

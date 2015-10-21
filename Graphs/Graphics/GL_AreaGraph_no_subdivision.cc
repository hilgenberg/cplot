#include "GL_AreaGraph.h"
#include "../Geometry/Axis.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "VisibilityFlags.h"
#include <GL/gl.h>
#include <vector>
#include <iostream>

//----------------------------------------------------------------------------------------------------------------------
// Worker threads
//----------------------------------------------------------------------------------------------------------------------

static void gridWorker(ThreadInfo &ti, int i1, int i2, GL_Mesh  &mesh, bool *eau, VisibilityFlags *vis,
					   bool flat, bool between, size_t &skipped_faces)
{
	// if between is true, we are on the second run through and the outer rows are already done
	
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------

	P3f    *vau = mesh.points();
	P3f    *nau = mesh.normals();
	GLuint *fau = mesh.faces();
	P2f    *tau = mesh.texture();

	bool do_normals = (nau != NULL);
	bool texture    = (tau != NULL);
	bool grid       = (eau != NULL);

	const DI_Axis        &ia = ti.ia;
	const DI_Grid        &ig = ti.ig;
	const DI_Subdivision &is = ti.is;
	
	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	bool disco = is.detect_discontinuities;

	//------------------------------------------------------------------------------------------------------------------
	// do the work, one row at a time
	//------------------------------------------------------------------------------------------------------------------

	GLuint     *face = fau + 3*(i1 * 2 * (nx-1));
	bool       *edge = eau + 3*(i1 * 2 * (nx-1));
	P3f *face_normal = nau + i1 * 2 * (nx-1); // used only if flat is true
	int iend = std::min(i2, ny-1);
	for (int i = i1 + (between ? 1 : 0); i <= iend; ++i)
	{
		double yi = ig.y[i];
		
		for (int j = 0; j < nx; ++j)
		{
			double xj = ig.x[j];
			int idx = nx*i+j; // vertex index
			
			if (!(between && i == i2))
			{
				//------------------------------------------------------------------------------------------------------
				// calculate the vertex and texture coordinates
				//------------------------------------------------------------------------------------------------------
				
				bool exists;
				ti.eval(xj, yi, vau[idx], exists, false, j>0 && !disco);
				
				if (exists)
				{
					vis[idx].set(ia, vau[idx]);
					if (texture) ti.ia.map_texture(xj, yi, tau[idx]);
				}
				else
				{
					vis[idx].set_invalid();
				}
			}
			
			if (i == i1 || j == 0) continue; // skip the faces until we have enough data
			
			//----------------------------------------------------------------------------------------------------------
			// do the two faces: count skipped faces and update normals
			//----------------------------------------------------------------------------------------------------------
			//  (A) --- (B)     if i+j is even, else the other diagonal
			//   |     / |      Faces are oriented ccw
			//   |   /   |
			//   | /     |
			//  (C) --- (D)

			// first see if the face should be drawn and if so, update vertex normals and edge flags
			int A = idx - 1, B = A+1, C = idx - nx - 1, D = C+1;
			int p1[2], p2[2], p3[2];
			int ei[4];
			bool even = ((i+j) & 1);
			if (even)
			{
				p1[0] = B;
				p2[0] = A;
				p3[0] = C;

				p1[1] = C;
				p2[1] = D;
				p3[1] = B;

				// horizontal edges are first
				ei[0] = i;
				ei[1] = j-1;
				ei[2] = i-1;
				ei[3] = j;
			}
			else
			{
				p1[0] = D;
				p2[0] = B;
				p3[0] = A;
				
				p1[1] = A;
				p2[1] = C;
				p3[1] = D;

				// vertical edges are first
				ei[0] = j;
				ei[1] = i;
				ei[2] = j-1;
				ei[3] = i-1;
			}
			
			if (disco)
			{
				double xm = 0.5*(xj + ig.x[j-1]), ym = 0.5*(yi + ig.y[i-1]);
				P3f  pm;
				bool exists;
				ti.eval(xm, ym, pm, exists);
				if (!exists)
				{
					skipped_faces += 2;
					continue;
				}
				
				float l1 = (vau[A]-pm).absq();
				float l2 = (vau[B]-pm).absq();
				float l3 = (vau[C]-pm).absq();
				float l4 = (vau[D]-pm).absq();
				float lmin = std::min(std::min(l1,l2), std::min(l3,l4));
				float lmax = std::max(std::max(l1,l2), std::max(l3,l4));
				if (lmax > 100.0f*lmin || lmax > (float)is.disco_limit)
				{
					skipped_faces += 2;
					continue;
				}
			}

			
			for (int k = 0; k < 2; ++k)
			{
				if (!VisibilityFlags::visible(vis[p1[k]], vis[p2[k]], vis[p3[k]]))
				{
					++skipped_faces;
					continue;
				}
				
				*face++ = p1[k];
				*face++ = p2[k];
				*face++ = p3[k];
				
				if (do_normals)
				{
					P3f d1, d2, n;
					sub(d1, vau[p2[k]], vau[p1[k]]);
					sub(d2, vau[p3[k]], vau[p1[k]]);
					cross(n, d1, d2);
					
					if (flat)
					{
						n.to_unit();
						*face_normal++ = n;
					}
					else
					{
						nau[p1[k]] += n;
						nau[p2[k]] += n;
						nau[p3[k]] += n;
					}
				}
				
				if (grid)
				{
					*edge++ = (even ? ig.y.visible(ei[2*k  ]) : ig.x.visible(ei[2*k  ]));
					*edge++ = (even ? ig.x.visible(ei[2*k+1]) : ig.y.visible(ei[2*k+1]));
					*edge++ = false; // diagonal
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Main method
//----------------------------------------------------------------------------------------------------------------------

void GL_AreaGraph::update_without_subdivision(int n_threads, std::vector<void *> &info)
{
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------

	const DI_Axis &ia = *(const DI_Axis*)info[1];
	const DI_Grid &ig = *(const DI_Grid*)info[3];

	int nx = ig.x.nlines();
	int ny = ig.y.nlines();
	
	size_t nvertexes = (size_t)nx * ny;
	size_t nfaces    = (size_t)(nx-1) * (ny-1) * 2;
	
	//------------------------------------------------------------------------------------------------------------------
	// allocate the arrays, sizing for every vertex and face being visible
	//------------------------------------------------------------------------------------------------------------------
	
	bool hiddenline = (graph.options.shading_mode == Shading_Hiddenline);
	bool wireframe  = (graph.options.shading_mode == Shading_Wireframe);
	bool flat       = (graph.options.shading_mode == Shading_Flat);
	bool do_normals = (!ia.is2D && !hiddenline && !wireframe);
	bool grid       = (wireframe || graph.options.grid_style == Grid_On);
	bool texture    = !wireframe; // this could be more precise but then the settingsBox pays for it...

	GL_Mesh::NormalMode nm = GL_Mesh::NormalMode::None;
	if (!ia.is2D) switch (graph.options.shading_mode)
	{
		case Shading_Flat:   nm = GL_Mesh::NormalMode::Face;   break;
		case Shading_Smooth: nm = GL_Mesh::NormalMode::Vertex; break;
		default: break;
	}
	mesh.resize(nvertexes, nfaces, nm, texture);
	
	std::unique_ptr<bool[]> eau(grid ? new bool[3*nfaces] : NULL);
	std::unique_ptr<VisibilityFlags[]> vis(new VisibilityFlags[nvertexes]);
	std::vector<size_t> skipped_faces;

	//------------------------------------------------------------------------------------------------------------------
	// create and run the worker threads
	//------------------------------------------------------------------------------------------------------------------

	Task task(&info);
	WorkLayer *gridLayer = new WorkLayer("grid", &task, NULL, 1);
	if (ia.S1) gridLayer->set_cyclic();
	
	int chunk = (ny+2*n_threads-1) / (2*n_threads);
	if (chunk < 1) chunk = 1;
	skipped_faces.resize((ny+chunk-1) / chunk);
	bool between = false;
	for (int i = 0, j = 0; i < ny; i += chunk, ++j, between = !between)
	{
		gridLayer->add_unit([=,&eau,&vis,&skipped_faces](void *ti)
		{
			gridWorker(*(ThreadInfo*)ti,
					   i, i+chunk, // i+chunk can be > ny, but the last worker-thread needs to know that it is the last
					   mesh, eau.get(), vis.get(),
					   flat, between, skipped_faces[j]);
		});
	}
	task.run(n_threads);

	//------------------------------------------------------------------------------------------------------------------
	// glue S1 x S1 together
	//------------------------------------------------------------------------------------------------------------------

	if (ia.S1)
	{
		P3f *vau = mesh.points();
		P3f *nau = mesh.normals();
		
		// top and bottom
		for (int i = 0; i < nx; ++i)
		{
			int j = nx*(ny-1)+i;
			if (vis[i].valid() && vis[j].valid())
			{
				vau[j] = vau[i];

				if (do_normals && !flat)
				{
					nau[i] += nau[j];
					nau[j]  = nau[i];
				}
			}
		}
		// left and right
		for (int k = 0; k < ny; ++k)
		{
			int i = k*nx;
			int j = k*nx + nx-1;
			if (vis[i].valid() && vis[j].valid())
			{
				vau[j] = vau[i];
				
				if (do_normals && !flat)
				{
					nau[i] += nau[j];
					nau[j]  = nau[i];
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// close gaps in the faces, edge flags and normal (if flatshaded) arrays
	//------------------------------------------------------------------------------------------------------------------

	mesh.close_gaps(2*(nx-1)*chunk, skipped_faces, eau.get());
	mesh.set_grid(eau.get());
	
#ifdef AGDEBUG
	std::cerr << " -- faces: " << nfaces << " + " << ((size_t)(nx-1) * (ny-1) * 2 - nfaces) << " dropped"
	          << ", grid: " << nx << " x " << ny
	          << ", vertices: " << nvertexes << std::endl;
#endif
}

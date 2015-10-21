#include "GL_ColorGraph.h"
#include "../../Utility/MemoryPool.h"
#include "../Threading/ThreadInfo.h"
#include "../Threading/ThreadMap.h"
#include "../OpenGL/GL_Util.h"
#include "../OpenGL/GL_RM.h"

#include <vector>
#include <algorithm>

//----------------------------------------------------------------------------------------------------------------------
// update worker: fill in the area [x1,x2) x [y1,y2)
//----------------------------------------------------------------------------------------------------------------------

static void update(ThreadInfo &ti, int w, int h, int y1, int y2, unsigned char *data, const GL_Image &tex, TextureProjection tp)
{
	const DI_Axis &ia = ti.ia;
	const DI_Calc &ic = ti.ic;
	BoundContext  &ec = ti.ec;
	
	int32_t *dst = (int32_t*)data;
	
	unsigned tw = tex.w(), th = tex.h();
	double ys = (double)tw / th;
	const int32_t *td = (int32_t*)tex.data().data();
	
	double tr = M_1_PI * 0.5 * std::min(tw-1, th-1) * 0.99999, tx = 0.5*(tw-1), ty = 0.5*(th-1); // for riemann
	double int_part; // unused
	
	for (int i = y1; i < y2; ++i)
	{
		int32_t *d = dst + (size_t)w * i;
		double y = ((h-1-i) * ia.min[1] + i * ia.max[1]) / (h-1);
		
		for (int j = 0; j < w; ++j, ++d)
		{
			double x = ((w-1-j) * ia.min[0] + j * ia.max[0]) / (w-1);
			
			cnum z;
			
			if (ic.complex)
			{
				if (ic.xi >= 0) ec.set_input(ic.xi, cnum(x,y));
				ec.eval();
				z = ec.output(0);
			}
			else
			{
				if (ic.xi >= 0) ec.set_input(ic.xi, x);
				if (j == 0 && ic.yi >= 0) ec.set_input(ic.yi, y);
				#ifdef DEBUG
				if (j != 0 && ic.yi >= 0) assert(ec.input(ic.yi).real() == y);
				#endif
				ec.eval();
				assert(ic.dim == 2);
				
				const cnum &xc = ec.output(0);
				const cnum &yc = ec.output(1);
				if (is_real(xc) && is_real(yc))
				{
					z.real(xc.real());
					z.imag(yc.real());
				}
				else
				{
					z = UNDEFINED;
				}
			}
			
			if (defined(z))
			{
				unsigned ti, tj;
				
				switch (tp)
				{
					case TP_Repeat:
					{
						double fx = modf(z.real(),    &int_part); if (fx < 0.0){ ++fx; assert(fx >= 0.0); }
						double fy = modf(z.imag()*ys, &int_part); if (fy < 0.0){ ++fy; assert(fy >= 0.0); }
						tj = (unsigned)(fx * tw) % tw;
						ti = th-1 - (unsigned)(fy * th) % th;
						assert(tj < tw);
						assert(ti < th);
						*d = td[(size_t)tw * ti + tj];
						break;
					}
						
					case TP_Center:
					{
						double x =  z.real();
						double y = -z.imag() * tw / th;
						if (abs(x) <= 1.0 && abs(y) <= 1.0)
						{
							tj = (unsigned)((x+1.0) * (tw-1) * 0.5);
							ti = (unsigned)((y+1.0) * (th-1) * 0.5);
							assert(tj < tw);
							assert(ti < th);
							*d = td[(size_t)tw * ti + tj];
						}
						else
						{
							*d = 0;
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
						 **********************************************************************************************/
						double r = abs(z);
						if (r > 0.0)
						{
							double f = tr * acos(2.0 / (r*r + 1.0) - 1.0) / r;
							tj = (unsigned)(tx + f*z.real());
							ti = (unsigned)(ty - f*z.imag());
						}
						else
						{
							tj = (unsigned)tx;
							ti = (unsigned)ty;
						}
						assert(tj < tw);
						assert(ti < th);
						*d = td[(size_t)tw * ti + tj];
						break;
						
						#if 0
						// TODO: is this riemann texture projection?
						double fx = std::min(1.0, th), fy = std::min(1.0, 1.0/th);
						for (int ip = 0; ip < 5; ++ip)
						{
							P3f &v = ...;
							double d = sqrt(2.0+2.0*v.z);
							t->x = (float)(0.5 + 0.5*fx * v.x/d);
							t->y = (float)(0.5 - 0.5*fy * v.y/d);
							++t;
						}
						#endif
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
						 x = theta*w/2π
						 y = phi*h/π
						 **********************************************************************************************/
						
						double phi   = acos(2.0 / (absq(z) + 1.0) - 1.0) / M_PI;
						double theta = atan2(z.imag(), z.real()) / M_PI + 1.0;
						
						tj = (unsigned)(theta * tw * 0.5) % tw;
						ti = (unsigned)(phi * th) % th;
						
						assert(tj < tw);
						assert(ti < th);
						*d = td[(size_t)tw * ti + tj];
						break;
					}
				}
				
			}
			else
			{
				*d = 0;
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update and draw
//----------------------------------------------------------------------------------------------------------------------

void GL_ColorGraph::update(int nthreads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim <= 0 || ic.dim > 3 || graph.options.texture.empty()){ im.redim(0, 0); return; }
	
	DI_Axis ia(graph, true, false);
	
	assert(graph.type() == C_C || graph.type() == R2_R2);
	assert(graph.isColor());
	
	DI_Grid        ig;
	DI_Subdivision is;
	
	std::vector<void *> info;
	info.push_back(&ic);
	info.push_back(&ia);
	info.push_back(&is);
	info.push_back(&ig);
	
	P3f range;
	ia.map_vector(P3d(ia.range[0], ia.range[1], ia.center[2]), range);
	xr = range.x;
	yr = range.y;
	ia.map(P3d(ia.center[0], ia.center[1], ia.min[2]), range);
	zr = range.z;

	Task task(&info);
	
	//------------------------------------------------------------------------------------------------------------------
	// (2) calculation
	//------------------------------------------------------------------------------------------------------------------
	
	unsigned char *data = NULL;
	try
	{
		int    w = graph.plot.camera.screen_w();
		int    h = graph.plot.camera.screen_h();
		double q = quality;
		int   w0 = std::max(1, w/64);
		int   h0 = std::max(1, h/64);
		int   w1 = std::max(w0, 2*w);
		int   h1 = std::max(h0, 2*h);
		data = im.redim((int)(w0+q*(w1-w0)), (int)(h0+q*(h1-h0)));
	}
	catch(...)
	{
		im.redim(0, 0);
	}
	if (im.empty()) return;

	WorkLayer *layer = new WorkLayer("calculate", &task, NULL);
	
	int     h = im.h();
	int chunk = (h+nthreads-1) / nthreads;
	if (chunk < 2) chunk = 2;
	for (int i = 0, j = 0; i < h; i += chunk, ++j)
	{
		int i1 = std::min(h, i+chunk);
		layer->add_unit([=](void *ti)
		{
			::update(*(ThreadInfo*)ti, im.w(), im.h(), i, i1, data, graph.options.texture, graph.options.texture_projection);
		});
	}
	
	task.run(nthreads);
}

Opacity GL_ColorGraph::opacity() const
{
	return  graph.options.transparency.off ||
	       !graph.options.transparency.forces_transparency() && graph.options.texture.opaque() ? OPAQUE : TRANSPARENT;
}

void GL_ColorGraph::draw(GL_RM &rm) const
{
	if (im.empty()) return;
	
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------
	bool is2d = (graph.plot.axis.type() == Axis::Rect);

	//------------------------------------------------------------------------------------------------------------------
	// setup light and textures
	//------------------------------------------------------------------------------------------------------------------
	
	glShadeModel(GL_SMOOTH);
	graph.plot.axis.options.light.on(0.0f); // does nothing in 2d
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	
	if (!graph.options.transparency.off)
	{
		rm.upload_texture(im);
		rm.setup(true, false, false);
		graph.options.transparency.set();
		glDepthMask(!graph.options.transparency.forces_transparency() && graph.options.texture.opaque());
		#ifdef DEBUG
		GLboolean dw; glGetBooleanv(GL_DEPTH_WRITEMASK, &dw);
		assert(!dw || im.opaque());
		#endif
	}
	else
	{
		GL_Color c = graph.plot.axis.options.background_color; c.a = 1.0f;
		rm.upload_texture(im, 1.0f, c);
		rm.setup(true, false, false);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	//------------------------------------------------------------------------------------------------------------------
	// draw triangles
	//------------------------------------------------------------------------------------------------------------------
	
	start_drawing();
	
	float z = is2d ? 0.0f : zr;
	glBegin(GL_QUADS);
	glNormal3f  (0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-xr, -yr, z);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-xr,  yr, z);
	glTexCoord2f(1.0f, 1.0f); glVertex3f( xr,  yr, z);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( xr, -yr, z);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	
	finish_drawing();
}

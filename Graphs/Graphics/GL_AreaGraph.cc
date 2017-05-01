#include "GL_AreaGraph.h"
#include "../Threading/ThreadInfo.h"
#include "../../Utility/System.h"
#include "../OpenGL/GL_Context.h"
#include <GL/gl.h>
#include <vector>

//----------------------------------------------------------------------------------------------------------------------
// AreaGraph update and drawing
//----------------------------------------------------------------------------------------------------------------------

static inline double sqr(double x){ return x*x; }

void GL_AreaGraph::update(int n_threads, double quality)
{
	//------------------------------------------------------------------------------------------------------------------
	// setup the info structs
	//------------------------------------------------------------------------------------------------------------------
	
	DI_Calc ic(graph);
	if (!ic.e0 || ic.dim == 0 || ic.dim > 3) return;
	
	bool circle, parametric;
	switch (graph.type())
	{
		case  C_C:  circle = false; parametric = (graph.mode()==GM_Image || graph.mode()==GM_Riemann); break;
		case R2_R:  circle = false; parametric = false; break;
		case R2_R3: circle = false; parametric = true;  break;
		case S2_R3: circle = true;  parametric = true;  break;
		case R2_R2: circle = false; parametric = (graph.mode()==GM_Image);  break;
		default: return;
	}
	
	DI_Axis ia(graph, !parametric, circle);
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
	is.disco_limit = sqr(150.0 * ia.pixel / ia.range[0]);
	is.max_lenq    = sqr(  5.0 * ia.pixel / ia.range[0]);
	is.min_lenq    = sqr(  1.0 * ia.pixel / ia.range[0]);

	//info.max_area     = sqr(4.0*info.pixel);
	//info.min_area     = sqr(2.0*info.pixel);
	is.max_kink  = sqr(1.0*ia.pixel);
	is.max_faces = (size_t)((quality*q0*1000.0 + 1.0)*1000.0);

	
	int ngrid = (int)ceil(sqrt(is.max_faces));
	if (ngrid < 12) ngrid = 12;
	DI_Grid ig(ia, graph.options.grid_density, ngrid, false);

	//int nx = ig.x.nlines(), ny = ig.y.nlines();
	//double dyn = graph.options.dynamic;
	//int depth = (int)round(dyn*10.0);
	
	/*is.max_lenq = sqr(8.3 / std::max(nx,ny));
	is.max_kink = is.max_lenq * 1e-6;*/
	
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
	// call the real update method
	//------------------------------------------------------------------------------------------------------------------

	/*if (depth <= 0)
	{
#ifdef DEBUG
		std::cerr << "AreaGraph update without subdivision, qd = (" << q0 << "*" << quality << ", dyn = " << graph.options.dynamic << ")" << std::endl;
#endif*/
		update_without_subdivision(n_threads, info);
	/*}
	else
	{
#ifdef DEBUG
		std::cerr << "AreaGraph update with subdivision, qd = (" << q0 << "*" << quality << ", dyn = " << graph.options.dynamic << ")" << std::endl;
#endif
		return update_with_subdivision(n_threads, depth, info);
	}*/
}

Opacity GL_AreaGraph::opacity() const
{
	if (graph.mode() == GM_RiemannColor)
	{
		return graph.options.transparency.off || // sets bg.alpha=1 in that case
		       !graph.options.transparency.forces_transparency() && graph.options.texture.opaque()
		       ? OPAQUE : TRANSPARENT;
	}
	else if (graph.options.shading_mode == Shading_Wireframe)
	{
		return graph.options.line_color.opaque() ? ANTIALIASED : TRANSPARENT;
	}
	else
	{
		return graph.options.transparency.off ||
		       graph.options.fill_color.opaque() && !graph.options.transparency.forces_transparency()
		       ? OPAQUE : TRANSPARENT;
	}
}

bool GL_AreaGraph::needs_depth_sort() const
{
	if (graph.mode() == GM_RiemannColor)
	{
		return !graph.options.transparency.off &&
		(graph.options.transparency.forces_transparency() || !graph.options.texture.opaque()) &&
		!graph.options.transparency.symmetric();
	}
	else if (graph.options.shading_mode == Shading_Wireframe)
	{
		return graph.plot.options.aa_mode == AA_Lines || !graph.options.line_color.opaque();
	}
	else
	{
		// ignore the grid because it will be drawn on top of the fill!

		return !graph.options.transparency.off &&
		(!graph.options.fill_color.opaque() || graph.options.transparency.forces_transparency()) &&
		!graph.options.transparency.symmetric();
	}
}

void GL_AreaGraph::draw(GL_RM &rm) const
{
	GL_CHECK;
	
	//------------------------------------------------------------------------------------------------------------------
	// extract info
	//------------------------------------------------------------------------------------------------------------------
	bool isColor     = graph.mode() == GM_RiemannColor;
	bool isHisto     = graph.mode() == GM_Histogram;
	bool hiddenline  = !isColor && graph.options.shading_mode == Shading_Hiddenline;
	bool wireframe   = !isColor && graph.options.shading_mode == Shading_Wireframe;
	bool flat        = !isColor && graph.options.shading_mode == Shading_Flat;
	bool is2d        = graph.plot.axis.type() == Axis::Rect;
	bool do_normals  = !is2d && !hiddenline && !wireframe;
	bool grid        = !isColor && (wireframe || graph.options.grid_style == Grid_On);
	bool full_grid   = !isColor && graph.options.grid_style == Grid_Full;
	bool texture     = isColor || (!wireframe && graph.options.texture_opacity > 1e-8 && !graph.options.texture.empty());
	bool envmap      = !isColor && do_normals && graph.options.reflection_opacity > 1e-8 && !graph.options.reflection_texture.empty();
	bool mask        = !isHisto && !isColor && graph.options.mask.style() != Mask_Off;
	bool transparent = !wireframe && !graph.options.fill_color.opaque();

	GL_Color fill_color = graph.options.fill_color;
	if (graph.options.transparency.off)
	{
		fill_color.a = 1.0f;
		assert(fill_color.opaque());
		transparent = false;
	}
	assert(wireframe || transparent == !fill_color.opaque());

	//------------------------------------------------------------------------------------------------------------------
	// setup light and textures
	//------------------------------------------------------------------------------------------------------------------
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	if (grid || full_grid)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
	}
	
	if (!do_normals)
	{
		graph.plot.axis.options.light.off();
	}
	else
	{
		graph.plot.axis.options.light.on(!isColor ? (float)graph.options.shinyness : 0.0f);
		if (texture) glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	}
	glShadeModel(flat ? GL_FLAT : GL_SMOOTH);

	GL_CHECK;

	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	if (mask)
	{
		glEnable(GL_TEXTURE_2D);
		graph.options.mask.upload(rm, mask_scale);
		glEnable(GL_ALPHA_TEST);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, mesh.texture());
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}

	GL_CHECK;

	glActiveTexture(GL_TEXTURE1);
	glClientActiveTexture(GL_TEXTURE1);
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		if (isColor)
		{
			if (graph.options.transparency.off)
			{
				GL_Color c = graph.plot.axis.options.background_color;
				c.a = 1.0f;
				rm.upload_texture(graph.options.texture, 1.0f, c);
			}
			else
			{
				rm.upload_texture(graph.options.texture);
			}
			rm.setup(graph.options.texture_projection == TP_Repeat, false, false);
		}
		else
		{
			rm.upload_texture(graph.options.texture, (float)graph.options.texture_opacity, fill_color);
			rm.setup(false, true, true);
		}
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, mesh.texture());
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}

	GL_CHECK;

	glActiveTexture(GL_TEXTURE2);
	glClientActiveTexture(GL_TEXTURE2);
	if (envmap)
	{
		glEnable(GL_TEXTURE_2D);
		rm.upload_texture(graph.options.reflection_texture, (float)graph.options.reflection_opacity);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glScaled(1.0, -1.0, 1.0);
	}
	else
	{
		glDisable(GL_TEXTURE_2D);
	}

	GL_CHECK;
	
	//------------------------------------------------------------------------------------------------------------------
	// draw triangles
	//------------------------------------------------------------------------------------------------------------------

	start_drawing();

	(texture ? GL_Color(1.0f) : fill_color).set();

	if (isColor)
	{
		graph.options.transparency.set();
		glDepthMask(graph.options.transparency.off ||
					!graph.options.transparency.forces_transparency() && graph.options.texture.opaque());
	}
	else if (wireframe)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(graph.plot.options.aa_mode != AA_Lines && graph.options.line_color.opaque());
	}
	else if (transparent)
	{
		graph.options.transparency.set();
		glDepthMask(GL_FALSE);
	}
	else if (graph.options.transparency.off)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_TRUE);
	}
	else
	{
		graph.options.transparency.set();
		glDepthMask(!graph.options.transparency.forces_transparency());
	}

	GL_CHECK;

	if (wants_backface_culling() && !needs_depth_sort())
	{
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	}

	if (!wireframe) mesh.draw(has_unit_normals());

	glDisable(GL_CULL_FACE);

	GL_CHECK;

	if (mask)
	{
		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);
	}
	if (envmap)
	{
		glActiveTexture(GL_TEXTURE2);
		glClientActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
	}
	if (texture)
	{
		glActiveTexture(GL_TEXTURE1);
		glClientActiveTexture(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	GL_CHECK;

	glClientActiveTexture(GL_TEXTURE0);
	glActiveTexture(GL_TEXTURE0);
	
	//------------------------------------------------------------------------------------------------------------------
	// draw grid
	//------------------------------------------------------------------------------------------------------------------

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);

	if (grid || full_grid)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (!wireframe)
		{
			GLboolean dw; glGetBooleanv(GL_DEPTH_WRITEMASK, &dw);
			glDepthMask(dw || graph.plot.options.aa_mode != AA_Lines && graph.options.grid_color.opaque() &&
			                  !graph.options.transparency.forces_transparency());
		}
		
		(wireframe ? graph.options.line_color : graph.options.grid_color).set();
		GLfloat lw = (GLfloat)(wireframe ? graph.options.line_width : graph.options.gridline_width);
		if (lw <= 0.01f) lw = 0.01f;
		glLineWidth(lw);
		mesh.draw_grid(full_grid);
	}
	
	GL_CHECK;
	
	//------------------------------------------------------------------------------------------------------------------
	// debugging
	//------------------------------------------------------------------------------------------------------------------

	bool draw_normals = false;
	settings.get("drawNormals", draw_normals);
	if (do_normals && draw_normals)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_TRUE);
		
		glColor3d(1.0, 0.0, 0.0);
		glLineWidth(1.0);

		mesh.draw_normals();
	}
		
	finish_drawing();
}

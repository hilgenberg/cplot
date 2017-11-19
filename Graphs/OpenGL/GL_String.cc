#include "GL_String.h"
#include "GL_Util.h"

#ifndef _WINDOWS
#include <GL/glext.h>
#include <pango/pangocairo.h>
#else
#include "../../Windows/stdafx.h"
#include <GdiPlus.h>
#pragma comment(lib,"gdiplus.lib")
#include <wingdi.h>
#endif

#define GT GL_TEXTURE_RECTANGLE
GL_String::GL_String(const std::string &text, const GL_Font &f) : texName(0), context(NULL), tex_w(-1)
{
	#ifdef _WINDOWS
	
	Gdiplus::Font font(Convert(f.name), f.size*4.0f);
	
	CString txt = Convert(text);

	// find text size
	int w, h;
	{
		CClientDC screenDC(NULL);
		Gdiplus::Graphics graphics(screenDC);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		graphics.SetPageUnit(Gdiplus::UnitPixel);
		Gdiplus::RectF rc;
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
		graphics.MeasureString(txt, txt.GetLength(), &font, Gdiplus::PointF(0.0f, 0.0f), &rc);
		Gdiplus::SizeF sz; rc.GetSize(&sz);
		w = (int)ceil(sz.Width); h = (int)ceil(sz.Height);
	}

	// draw text into CImage
	CImage im; im.CreateEx(w, h, 32, BI_RGB, NULL, CImage::createAlphaChannel);
	Gdiplus::Graphics graphics(im.GetDC());
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetPageUnit(Gdiplus::UnitPixel);
	Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
	graphics.DrawString(txt, txt.GetLength(), &font, Gdiplus::PointF(0.0f, 0.0f), &brush);
	im.ReleaseDC();

	// transfer CImage into GL texture
	assert(w == im.GetWidth());
	assert(h == im.GetHeight());
	assert(im.GetBits());
	assert(im.GetBPP() == 32);
	auto dr = im.GetPitch();
	char *bmp = (char *)im.GetBits();
	std::vector<unsigned> buf;
	if (dr != 4 * w)
	{
		buf.resize(w * h + 1);
		char *b = (char *)buf.data();
		for (int y = 0; y < h; ++y) memcpy(b + 4 * w*y, im.GetPixelAddress(0, y), 4 * w);
		bmp = (char*)buf.data();
	}

	tex_w = (GLfloat)w; tex_h = (GLfloat)h;
	frame_w = 0.25f*(float)w; frame_h = 0.25f*(float)h;

	glPushAttrib(GL_TEXTURE_BIT);
	glGenTextures(1, &texName);
	glBindTexture(GT, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GT, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, bmp);
	glPopAttrib();

	#else
	cairo_surface_t *tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
	cairo_t *layout_context = cairo_create(tmp); cairo_surface_destroy(tmp);

	PangoLayout *layout = pango_cairo_create_layout(layout_context);
	pango_layout_set_text(layout, text.c_str(), -1);

	std::string pf = format("%s %d", f.name.c_str(), (int)round(f.size));
	PangoFontDescription *desc = pango_font_description_from_string(pf.c_str());
	pango_layout_set_font_description(layout, desc); pango_font_description_free(desc);

	int w, h; pango_layout_get_size(layout, &w, &h);
	w /= PANGO_SCALE; h /= PANGO_SCALE; // to pixels
	tex_w = w; tex_h = h;
	frame_w = w; frame_h = h;
	unsigned char *bmp = new unsigned char[4*w*h];
	memset(bmp, 0, 4*w*h);

	cairo_surface_t *surface = cairo_image_surface_create_for_data(bmp, CAIRO_FORMAT_ARGB32, w, h, 4*w);
	cairo_t *render_context  = cairo_create(surface);

	auto &c = f.color;
	cairo_set_source_rgba(render_context, c.r, c.g, c.b, 1.0);
	pango_cairo_show_layout(render_context, layout);

	glPushAttrib(GL_TEXTURE_BIT);
	glGenTextures(1, &texName);
	glBindTexture(GT, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GT, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, bmp);
	glPopAttrib();

	g_object_unref(layout);
	cairo_destroy(layout_context);
	cairo_destroy(render_context);
	cairo_surface_destroy(surface);
	delete[] bmp;
	#endif
}

GL_String::~GL_String()
{
	if (texName) context.delete_texture(texName);
}

static inline void start_drawing(GLuint texName)
{
	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
	#ifdef _WINDOWS
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	#else
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	#endif
	
	glEnable(GT);
	glBindTexture (GT, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
}
static inline void finish_drawing()
{
	glDisable(GT);
	glPopAttrib();
}

void GL_String::draw(float x, float y, float w, float h)
{
	if (!texName) return;
	start_drawing(texName);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); glVertex3f(x,     0.0f, y + h);
	glTexCoord2f( 0.0f, tex_h); glVertex3f(x,     0.0f, y);
	glTexCoord2f(tex_w, tex_h); glVertex3f(x + w, 0.0f, y);
	glTexCoord2f(tex_w,  0.0f); glVertex3f(x + w, 0.0f, y + h);
	glEnd();
	finish_drawing();
}
void GL_String::draw2d(float x, float y, float w, float h)
{
	if (!texName) return;
	start_drawing(texName);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); glVertex2f(x,     y);
	glTexCoord2f( 0.0f, tex_h); glVertex2f(x,     y + h);
	glTexCoord2f(tex_w, tex_h); glVertex2f(x + w, y + h);
	glTexCoord2f(tex_w,  0.0f); glVertex2f(x + w, y);
	glEnd();
	finish_drawing();
}
void GL_String::draw(const P3f &p, const P3f &dx, const P3f &dy) // p is topleft
{
	if (!texName) return;
	start_drawing(texName);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); glVertex3fv(p+dy);
	glTexCoord2f( 0.0f, tex_h); glVertex3fv(p   );
	glTexCoord2f(tex_w, tex_h); glVertex3fv(p+dx);
	glTexCoord2f(tex_w,  0.0f); glVertex3fv(p+dx+dy);
	glEnd();
	finish_drawing();
}

void GL_String::draw(const P3d &p_, HorizontalPosition hp, VerticalPosition vp, const Axis &axis, double scale)
{
	if (!texName) return;
	double tw = scale*w(), th = scale*h();
	
	P3d p(p_);
	switch (hp)
	{
		case LEFT:    break;
		case HCENTER: p.x -= 0.5*tw; break;
		case RIGHT:   p.x -=     tw; break;
	}
	switch (vp)
	{
		case BOTTOM:  break;
		case VCENTER: p.y -= 0.5*th; break;
		case TOP:     p.y -=     th; break;
	}

	P3d q(p.x + tw, p.y + th, p.z);
	P3f pp, qq;
	axis.map(p, pp);
	axis.map(q, qq); // pp.z = qq.z = 0

	start_drawing(texName);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); glVertex3f (pp.x, qq.y, 0.0f);
	glTexCoord2f( 0.0f, tex_h); glVertex3fv(pp);
	glTexCoord2f(tex_w, tex_h); glVertex3f (qq.x, pp.y, 0.0f);
	glTexCoord2f(tex_w,  0.0f); glVertex3fv(qq);
	glEnd();
	finish_drawing();
}
void GL_String::draw(const P3d &p_, const P3d &dx, const P3d &dy, HorizontalPosition hp, VerticalPosition vp, const Axis &axis, double scale)
{
	if (!texName) return;
	double tw = scale*w(), th = scale*h();
	
	P3d p(p_);
	switch (hp)
	{
		case LEFT:    break;
		case HCENTER: p -= dx*(0.5*tw); break;
		case RIGHT:   p -= dx*tw; break;
	}
	switch (vp)
	{
		case BOTTOM:  break;
		case VCENTER: p -= dy*(0.5*th); break;
		case TOP:     p -= dy*th; break;
	}
	
	start_drawing(texName);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); vertex(p+dy*th, axis);
	glTexCoord2f( 0.0f, tex_h); vertex(p, axis);
	glTexCoord2f(tex_w, tex_h); vertex(p+dx*tw, axis);
	glTexCoord2f(tex_w,  0.0f); vertex(p+dx*tw+dy*th, axis);
	glEnd();
	finish_drawing();

}
void GL_String::draw_fixed(const P3d &p_, const P3d &dx, const P3d &dy, HorizontalPosition hp, VerticalPosition vp, double scale)
{
	if (!texName) return;
	double tw = scale*w(), th = scale*h();
	
	P3d p(p_);
	switch (hp)
	{
		case LEFT:    break;
		case HCENTER: p -= dx*(0.5*tw); break;
		case RIGHT:   p -= dx*tw; break;
	}
	switch (vp)
	{
		case BOTTOM:  break;
		case VCENTER: p -= dy*(0.5*th); break;
		case TOP:     p -= dy*th; break;
	}
	
	start_drawing(texName);
	glBegin(GL_QUADS);
	glTexCoord2f( 0.0f,  0.0f); glVertex3dv(p+dy*th);
	glTexCoord2f( 0.0f, tex_h); glVertex3dv(p);
	glTexCoord2f(tex_w, tex_h); glVertex3dv(p+dx*tw);
	glTexCoord2f(tex_w,  0.0f); glVertex3dv(p+dx*tw+dy*th);
	glEnd();
	finish_drawing();
	
}


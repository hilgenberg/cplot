#include "GL_String.h"
#include "GL_Util.h"

//#define FONT "Sans 18"
//#define TEXT "The quick brown fox is so かわいい!"
#define GT GL_TEXTURE_RECTANGLE
GL_String::GL_String(const std::string &text, const GL_Font &f) : texName(0), context(NULL), tex_w(-1)
{
}

GL_String::~GL_String()
{
	if (texName) context.delete_texture(texName);
}

static inline void start_drawing(GLuint texName)
{
}
static inline void finish_drawing()
{
}

void GL_String::draw(float x, float y, float w, float h)
{
}
void GL_String::draw2d(float x, float y, float w, float h)
{
}
void GL_String::draw(const P3f &p, const P3f &dx, const P3f &dy) // p is topleft
{
}

void GL_String::draw(const P3d &p_, HorizontalPosition hp, VerticalPosition vp, const Axis &axis, double scale)
{
}
void GL_String::draw(const P3d &p_, const P3d &dx, const P3d &dy, HorizontalPosition hp, VerticalPosition vp, const Axis &axis, double scale)
{
}
void GL_String::draw_fixed(const P3d &p_, const P3d &dx, const P3d &dy, HorizontalPosition hp, VerticalPosition vp, double scale)
{
}


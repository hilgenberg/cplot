#pragma once
#include "GL_Font.h"
#include "GL_Context.h"
#include "../Geometry/Axis.h"
#include <string>
#include <GL/gl.h>

enum VerticalPosition  { TOP  = -1, VCENTER = 0, BOTTOM = 1 };
enum HorizontalPosition{ LEFT = -1, HCENTER = 0, RIGHT  = 1 };

class GL_String
{
public:
	GL_String(const std::string &s, const GL_Font &f);
	~GL_String();

	GL_String(const GL_String &) = delete;
	GL_String &operator=(const GL_String &) = delete;

	void draw(float x, float y, float w, float h); // GL-coordinates (x,y=0,z)
	void draw2d(float x, float y, float w, float h); // GL-coordinates (x,y,z=0)
	void draw(const P3f &top_left, const P3f &dx, const P3f &dy);

	void draw(const P3d &p, HorizontalPosition hp, VerticalPosition vp, const Axis &axis, double scale); // axis-coordinates
	void draw(const P3d &p, const P3d &dx, const P3d &dy, HorizontalPosition hp, VerticalPosition vp, const Axis &axis, double scale); // dx and dy must be unit vectors
	void draw_fixed(const P3d &p, const P3d &dx, const P3d &dy, HorizontalPosition hp, VerticalPosition vp, double scale); // dx and dy must be unit vectors

	float w() const{ return frame_w; }
	float h() const{ return frame_h; }
	
private:
	GL_Context context; // context that contains the texture
	GLuint     texName;
	GLfloat    tex_w, tex_h;     // size of the texture
	float      frame_w, frame_h; // size of the string
};

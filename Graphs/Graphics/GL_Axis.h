#pragma once
#include "../Geometry/Axis.h"
#include "../Geometry/Camera.h"
#include <map>
#include <string>
#include "../OpenGL/GL_String.h"
#include "../OpenGL/GL_StringCache.h"
struct Plot;

class GL_Axis
{
public:
	GL_Axis(Axis &axis, Camera &camera) : axis(axis), camera(camera){ }
	void draw(const Plot &plot) const;

private:
	Axis   &axis;
	Camera &camera;
	mutable GL_StringCache labelCache;
	
	void draw2D() const;
	void draw3D() const;
	void draw3D_cross() const;
	void drawSphere() const;
};

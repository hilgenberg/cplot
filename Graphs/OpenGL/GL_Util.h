#pragma once
#include "../Geometry/Vector.h"
#include "../Geometry/Axis.h"
#include <GL/gl.h>

void draw_arrow(const P3d &p, const P3d &u, double max_tip_length, const Axis &axis); // arrow from p to p+u
void draw_arrow(const P2d &p, const P2d &u, double max_tip_length, const Axis &axis);

void draw_arrow3d(const P3f &p, const P3f &u, float max_tip_length); // arrow from p to p+u
void draw_arrow2d(const P3f &p, const P3f &u, float max_tip_length);


inline void vertex(const P3d &p, const Axis &axis)
{
	P3f q;
	axis.map(p, q);
	glVertex3fv(q);
}
inline void vertex(double x, double y, double z, const Axis &axis)
{
	P3f q;
	axis.map(P3d(x,y,z), q);
	glVertex3fv(q);
}

inline void vertex2d(const P2d &p, const Axis &axis)
{
	P3f q;
	axis.map(P3d(p.x, p.y, 0.0), q);
	glVertex3fv(q);
}


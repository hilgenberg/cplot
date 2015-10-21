#include "GL_Util.h"

void draw_arrow3d(const P3f &p, const P3f &u_, float max_tip_length)
{
	P3f q(p+u_);
	
	// draw the line
	glBegin(GL_LINES);
	glVertex3fv(p);
	glVertex3fv(q);
	glEnd();
	
	float r  = u_.absq();
	float rr = sqrtf(r);
	//if (rr < max_tip_length*0.1f) return; // too short for drawing the tip
	
	P3f v, w, u(u_);
	u /= rr;
	float rx = r - u_.x*u_.x;
	float ry = r - u_.y*u_.y;
	float rz = r - u_.z*u_.z;
	if (rz > rx && rz > ry)
	{
		float rrz = sqrtf(rz);
		// arrow is closest to XY-plane, find a unit vector v in that plane, that is orthogonal to u
		v.set(-u_.y/rrz, u_.x/rrz, 0.0f);
		// get another unit vector that is orthogonal to u and v
		w.set(-v.y*u.z, v.x*u.z, rrz/rr);
	}
	else if (rx > ry)
	{
		// YZ-plane
		float rrx = sqrtf(rx);
		v.set(0.0f, -u_.z/rrx, u_.y/rrx);
		w.set(rrx/rr, -v.z*u.x, v.y*u.x);
	}
	else
	{
		// XZ-plane
		float rry = sqrtf(ry);
		v.set(-u_.z/rry, 0.0f, u_.x/rry);
		w.set(-v.z*u.y, rry/rr, v.x*u.y);
	}
	
	float len   = std::min(rr*0.5f, max_tip_length);
	float width = len * 0.15f;
	float inset =  len * 0.02f; // inset factor for base
	
	P3f b(q - u*len);
	
	// tip
	glBegin(GL_TRIANGLE_FAN);
	glEdgeFlag(GL_TRUE);
	glVertex3fv(q);
	glVertex3f(b.x + width*(v.x + w.x), b.y + width*(v.y + w.y), b.z + width*(v.z + w.z));
	glVertex3f(b.x + width*(v.x - w.x), b.y + width*(v.y - w.y), b.z + width*(v.z - w.z));
	glVertex3f(b.x - width*(v.x + w.x), b.y - width*(v.y + w.y), b.z - width*(v.z + w.z));
	glVertex3f(b.x - width*(v.x - w.x), b.y - width*(v.y - w.y), b.z - width*(v.z - w.z));
	glVertex3f(b.x + width*(v.x + w.x), b.y + width*(v.y + w.y), b.z + width*(v.z + w.z)); // first again
	glEnd();
	
	// base of tip
	if (len > 0.5f*max_tip_length)
	{
		glBegin(GL_TRIANGLE_FAN);
		glEdgeFlag(GL_TRUE);
		glVertex3f(b.x + u.x*inset, b.y + u.y*inset, b.z + u.z*inset);
		// reverse order from above, so the inside/outside is consistent, though we don't use that yet
		glVertex3f(b.x - width*(v.x - w.x), b.y - width*(v.y - w.y), b.z - width*(v.z - w.z));
		glVertex3f(b.x - width*(v.x + w.x), b.y - width*(v.y + w.y), b.z - width*(v.z + w.z));
		glVertex3f(b.x + width*(v.x - w.x), b.y + width*(v.y - w.y), b.z + width*(v.z - w.z));
		glVertex3f(b.x + width*(v.x + w.x), b.y + width*(v.y + w.y), b.z + width*(v.z + w.z));
		glVertex3f(b.x - width*(v.x - w.x), b.y - width*(v.y - w.y), b.z - width*(v.z - w.z)); // first again
		glEnd();
	}
}
void draw_arrow(const P3d &p, const P3d &u_, double max_tip_length, const Axis &axis)
{
	P3d q(p+u_);
	
	// draw the line
	glBegin(GL_LINES);
	vertex(p, axis);
	vertex(q, axis);
	glEnd();
	
	double r  = u_.absq();
	double rr = sqrt(r);
	if (rr < max_tip_length*0.1) return; // too short for drawing the tip
	
	P3d v, w, u(u_);
	u /= rr;
	double rx = r - u_.x*u_.x;
	double ry = r - u_.y*u_.y;
	double rz = r - u_.z*u_.z;
	if (rz > rx && rz > ry)
	{
		double rrz = sqrt(rz);
		// arrow is closest to XY-plane, find a unit vector v in that plane, that is orthogonal to u
		v.set(-u_.y/rrz, u_.x/rrz, 0.0);
		// get another unit vector that is orthogonal to u and v
		w.set(-v.y*u.z, v.x*u.z, rrz/rr);
	}
	else if (rx > ry)
	{
		// YZ-plane
		double rrx = sqrt(rx);
		v.set(0.0, -u_.z/rrx, u_.y/rrx);
		w.set(rrx/rr, -v.z*u.x, v.y*u.x);
	}
	else
	{
		// XZ-plane
		double rry = sqrt(ry);
		v.set(-u_.z/rry, 0.0, u_.x/rry);
		w.set(-v.z*u.y, rry/rr, v.x*u.y);
	}
	
	double len   = std::min(rr*0.25, max_tip_length);
	double width = len * 0.15;
	double inset =  len * 0.02; // inset factor for base
	
	P3d b(q - u*len);
	
	// tip
	glBegin(GL_TRIANGLE_FAN);
	glEdgeFlag(GL_TRUE);
	vertex(q, axis);
	vertex(b.x + width*(v.x + w.x), b.y + width*(v.y + w.y), b.z + width*(v.z + w.z), axis);
	vertex(b.x + width*(v.x - w.x), b.y + width*(v.y - w.y), b.z + width*(v.z - w.z), axis);
	vertex(b.x - width*(v.x + w.x), b.y - width*(v.y + w.y), b.z - width*(v.z + w.z), axis);
	vertex(b.x - width*(v.x - w.x), b.y - width*(v.y - w.y), b.z - width*(v.z - w.z), axis);
	vertex(b.x + width*(v.x + w.x), b.y + width*(v.y + w.y), b.z + width*(v.z + w.z), axis); // first again
	glEnd();
	
	// base of tip
	if (len > 0.5*max_tip_length)
	{
		glBegin(GL_TRIANGLE_FAN);
		glEdgeFlag(GL_TRUE);
		vertex(b.x + u.x*inset, b.y + u.y*inset, b.z + u.z*inset, axis);
		// reverse order from above, so the inside/outside is consistent, though we don't use that yet
		vertex(b.x - width*(v.x - w.x), b.y - width*(v.y - w.y), b.z - width*(v.z - w.z), axis);
		vertex(b.x - width*(v.x + w.x), b.y - width*(v.y + w.y), b.z - width*(v.z + w.z), axis);
		vertex(b.x + width*(v.x - w.x), b.y + width*(v.y - w.y), b.z + width*(v.z - w.z), axis);
		vertex(b.x + width*(v.x + w.x), b.y + width*(v.y + w.y), b.z + width*(v.z + w.z), axis);
		vertex(b.x - width*(v.x - w.x), b.y - width*(v.y - w.y), b.z - width*(v.z - w.z), axis); // first again
		glEnd();
	}
}

//----------------------------------------------------------------------------------------------------------------------

void draw_arrow2d(const P3f &p, const P3f &u_, float max_tip_length)
{
	P3f q(p+u_);
	
	float r  = u_.absq();
	float rr = sqrtf(r);
	//if (rr < max_tip_length*0.1f) return; // too short for drawing the tip
	
	P3f u(u_);
	u /= rr;
	//P3f v(u.z, u.y, -u.x);
	P3f v(u.y, -u.x, u.z);
	
	float len   = std::min(rr*0.5f, max_tip_length);
	float width = len * 0.25f;
	float inset =  len * 0.1f; // inset factor for base
	
	P3f b(q - u*len);
	// draw the line
	glBegin(GL_LINES);
	glVertex3fv(p);
	glVertex3fv(len > 0.5*max_tip_length ? b + u*inset : b);
	glEnd();
	
	// tip
	glBegin(GL_TRIANGLE_FAN);
	glEdgeFlag(GL_TRUE);
	glVertex3fv(q);
	glVertex3fv(b + v*width);
	if (len > 0.5*max_tip_length) glVertex3fv(b + u*inset);
	glVertex3fv(b - v*width);
	glEnd();
}

void draw_arrow(const P2d &p, const P2d &u_, double max_tip_length, const Axis &axis)
{
	P2d q(p+u_);
	
	// draw the line
	glBegin(GL_LINES);
	vertex2d(p, axis);
	vertex2d(q, axis);
	glEnd();
	
	double r  = u_.absq();
	double rr = sqrt(r);
	if (rr < max_tip_length*0.1) return; // too short for drawing the tip
	
	P2d u(u_);
	u /= rr;
	P2d v(u.y, -u.x);
	
	double len   = std::min(rr*0.25, max_tip_length);
	double width = len * 0.25;
	double inset =  len * 0.1; // inset factor for base
	
	P2d b(q - u*len);
	
	// tip
	glBegin(GL_TRIANGLE_FAN);
	glEdgeFlag(GL_TRUE);
	vertex2d(q, axis);
	vertex2d(b + v*width, axis);
	if (len > 0.5*max_tip_length) vertex2d(b + u*inset, axis);
	vertex2d(b - v*width, axis);
	glEnd();
}


/*
 static void draw_sphere(P3 &c, float r, int div)
 {
 glPushMatrix(); glTranslatef(c.x, c.y, c.z); glutSolidSphere(r, div, div); glPopMatrix();
 }
 
 static void rline(P2 &p1, P2 &p2, int div)
 {
 glBegin(GL_LINE_STRIP);
 for(int i=0; i<=div; ++i){
 P2 p; P3 q;
 set(p, ((div-i)*p1.x+i*p2.x)/div, ((div-i)*p1.y+i*p2.y)/div);
 rproj(p,q);
 glVertex3f(q.x, q.y, q.z);
 }
 glEnd();
 }*/


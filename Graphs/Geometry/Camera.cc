#include "Camera.h"
#include "Axis.h"
#include <GL/gl.h>
#include <GL/glu.h>

//----------------------------------------------------------------------------------------------------------------------
// antialiasing
//----------------------------------------------------------------------------------------------------------------------

static double jit4[4][2] = {{0.375, 0.25}, {0.125, 0.75}, {0.875, 0.25}, {0.625, 0.75}};
static double jit8[8][2] = {{0.5625, 0.4375}, {0.0625, 0.9375}, {0.3125, 0.6875}, {0.6875, 0.8125}, {0.8125, 0.1875}, {0.9375, 0.5625}, {0.4375, 0.0625}, {0.1875, 0.3125}};

/* accFrustum()
 * The first 6 arguments are identical to the glFrustum() call.
 *
 * pixdx and pixdy are anti-alias jitter in pixels.
 * Set both equal to 0.0 for no anti-alias jitter.
 * eyedx and eyedy are depth-of field jitter in pixels.
 * Set both equal to 0.0 for no depth of field effects.
 *
 * focus is distance from eye to plane in focus.
 * focus must be greater than, but not equal to 0.0.
 *
 * Note that accFrustum() calls glTranslatef().  You will
 * probably want to insure that your ModelView matrix has been
 * initialized to identity before calling accFrustum().
 */
void Camera::accFrustum(double left, double right, double bottom, double top, double near, double far, double pixdx, double pixdy, double eyedx, double eyedy, double focus) const
{
	double xwsize = right - left;
	double ywsize = top - bottom;
	
	double dx = -(pixdx*xwsize/(double)w + eyedx*near/focus);
	double dy = -(pixdy*ywsize/(double)h + eyedy*near/focus);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(left + dx, right + dx, bottom + dy, top + dy, near, far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(-eyedx, -eyedy, 0.0);
}

/* accPerspective()
 *
 * The first 4 arguments are identical to the gluPerspective() call.
 * pixdx and pixdy are anti-alias jitter in pixels.
 * Set both equal to 0.0 for no anti-alias jitter.
 * eyedx and eyedy are depth-of field jitter in pixels.
 * Set both equal to 0.0 for no depth of field effects.
 *
 * focus is distance from eye to plane in focus.
 * focus must be greater than, but not equal to 0.0.
 *
 * Note that accPerspective() calls accFrustum().
 */
void Camera::accPerspective(double fovy, double aspect, double near, double far, double pixdx, double pixdy, double eyedx, double eyedy, double focus) const
{
	double fov2 = ((fovy*M_PI) / 180.0) * 0.5;
	double top = near / (cos(fov2) / sin(fov2));
	double bottom = -top;
	double right = top * aspect;
	double left = -right;
	accFrustum(left, right, bottom, top, near, far, pixdx, pixdy, eyedx, eyedy, focus);
}
void Camera::accOrtho(double left, double right, double bottom, double top, double near, double far, double pixdx, double pixdy) const
{
	double xwsize = right - left;
	double ywsize = top - bottom;
	double dx = -(pixdx*xwsize/(double)w);
	double dy = -(pixdy*ywsize/(double)h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left+dx, right+dx, bottom+dy, top+dy, near, far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//----------------------------------------------------------------------------------------------------------------------
// Camera
//----------------------------------------------------------------------------------------------------------------------

const double Camera::znear = 0.01;
const double Camera::zfar  = 64.0;

void Camera::save(Serializer &s) const
{
	s.double_(z);
	s.double_(fov);
	s.bool_(upright);
	rot.save(s);
}
void Camera::load(Deserializer &s)
{
	s.double_(z);
	s.double_(fov);
	s.bool_(upright);
	rot.load(s);
}

void Camera::viewport(int w_, int h_, Axis &axis)
{
	w = w_;
	h = h_;
	hr = (double)h / w;
	axis.window(w, h);
}

void Camera::set(int aa_pass, int num_passes) const
{
	assert(num_passes == 1 || num_passes == 4 || num_passes == 8);
	assert(aa_pass >= 0 && aa_pass < num_passes);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (ortho)
	{
		if (num_passes == 1)
		{
			glOrtho(-1.0, 1.0, -hr, hr, -1.0, 1.0);
		}
		else if (num_passes == 4)
		{
			accOrtho(-1.0, 1.0, -hr, hr, -1.0, 1.0, jit4[aa_pass][0], jit4[aa_pass][1]);
		}
		else if (num_passes == 8)
		{
			accOrtho(-1.0, 1.0, -hr, hr, -1.0, 1.0, jit8[aa_pass][0], jit8[aa_pass][1]);
		}
	}
	else
	{
		// fov is fovx, [-1,1] on x will touch both sides of the view frustum for camera.z == 1
		// [-hr,hr] on z will touch top and bottom. This way our pixel size is ok at the origin.
		// Also when there's no rotation and camera.z == 1 then 2D stuff could be drawn in this
		// projection and it looks exactly like in the orthogonal projection.
		if (num_passes == 1)
		{
			gluPerspective(2.0*atan2(tan(fov*0.5),1.0/hr)/DEGREES, 1.0/hr, znear, zfar);
		}
		else if (num_passes == 4)
		{
			accPerspective(2.0*atan2(tan(fov*0.5),1.0/hr)/DEGREES, 1.0/hr, znear, zfar, jit4[aa_pass][0], jit4[aa_pass][1], 0.0, 0.0, 1.0);
		}
		else if (num_passes == 8)
		{
			accPerspective(2.0*atan2(tan(fov*0.5),1.0/hr)/DEGREES, 1.0/hr, znear, zfar, jit8[aa_pass][0], jit8[aa_pass][1], 0.0, 0.0, 1.0);
		}
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (ortho)
	{
		//glRotated(-90.0, 1.0, 0.0, 0.0);
	}
	else
	{
		zf = 1.0/tan(0.5*fov);
		glTranslated(0, 0, -z * zf);
		
		P3d axis;
		double angle = rot.axis_angle(axis);
		glRotated(-90.0, 1.0, 0.0, 0.0); // this is "the 90° rotation"
		glRotated(angle / DEGREES, axis.x, axis.y, axis.z);
	}
}

void Camera::move(Axis &axis, double dx, double dy, double dz, bool /*free*/)
{
	if (ortho)
	{
		axis.move(dx, dz, 0);
	}
	/*else if (upright)
	{
		//axis.move(dx, dy, dz);
		//P3d dp = rot[P3d(dx, -dy, dz)];
		axis.move(dx, dy, 0);
	}*/
	else
	{
		// inverse of the 90° rotation flips z, then swaps y and z
		P3d dp = rot[P3d(dx, -dz, dy)];
		axis.move(dp.x, dp.y, dp.z);
	}
}

void Camera::rotate(double dx, double dy, double dz, bool free)
{
	if (ortho) return;
	
	if (free)
	{
		rot.rotate(dx, dy, dz, true );
	}
	else
	{
		// snap upright
		/*P3d Z = rot(P3d(0,0,1));
		if (fabs(Z.x) < sin(3.0*M_PI/180.0))
		{
			P3d axis; cross(axis, Z, P3d(1,0,0));
			axis.to_unit();
			rot.rotate(axis, -asin(Z.x));
		}*/
		
		rot.rotate( 0,  0, dz, false);
		rot.rotate(dx, dy,  0, true );
	}
}

void Camera::zoom(double f)
{
	if (ortho) return; // zoom the axis instead
	z *= f;
	normalize();
}

void Camera::normalize()
{
	if (!std::isnormal(z) || z > 25.0) z = 25.0;
	else if (z < .1) z = .1;

	assert(fov > 0.0 && fov < M_PI);
	assert(z > 0.0);
	// used to normalize the rotation here
}

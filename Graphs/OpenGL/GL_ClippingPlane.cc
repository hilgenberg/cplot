#include "GL_ClippingPlane.h"
#include "../Geometry/Axis.h"

void GL_ClippingPlane::save(Serializer &s) const
{
	if (s.version() < FILE_VERSION_1_3)
	{
		if (!m_enabled) return;
		NEEDS_VERSION(FILE_VERSION_1_3, "custom clipping plane");
	}
	s.bool_(m_enabled);
	s.float_(m_normal.x);
	s.float_(m_normal.y);
	s.float_(m_normal.z);
	s.float_(m_distance);
	s.bool_(m_locked);
}
void GL_ClippingPlane::load(Deserializer &s)
{
	if (s.version() < FILE_VERSION_1_3)
	{
		m_normal.set(0.0f, 1.0f, 0.0f);
		m_distance = 0.0f;
		m_locked = m_enabled = false;
	}
	else
	{
		s.bool_(m_enabled);
		s.float_(m_normal.x);
		s.float_(m_normal.y);
		s.float_(m_normal.z);
		s.float_(m_distance);
		s.bool_(m_locked);
	}
}

static inline double dsphere(double d) // effective distance for riemann sphere
{
	d /= 1.73; // slider range
	return sin(M_PI_2*sin(M_PI_2*d));
}

void GL_ClippingPlane::set(const Axis &axis, GLenum planeID) const
{
	if (!on() || !planeID) return;
	if (m_id) unset();
	
	P4d cf(m_normal.x, m_normal.y, m_normal.z, -(axis.type() == Axis::Sphere ? dsphere(m_distance) : m_distance));
	glClipPlane(planeID, cf);
	glEnable(planeID);
	m_id = planeID;
}

void GL_ClippingPlane::unset() const
{
	if (!m_id) return;
	glDisable(m_id);
	m_id = 0;
}

static inline void drawLine(float x1, float y1, float z1, float x2, float y2, float z2)
{
	P3f p1(x1,y1,z1), p2(x2,y2,z2);
	glVertex3fv(p1);
	glVertex3fv(p2);
}

void GL_ClippingPlane::draw(const Axis &axis) const
{
	if (!on() || (axis.type() != Axis::Box && axis.type() != Axis::Sphere)) return;

	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.0f);
	axis.options.axis_color.set();
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(2, 0xAAAA);


	if (axis.type() == Axis::Sphere)
	{
		// points on the clipping plane fulfill p*n = d
		P3d    n(m_normal);
		double d(dsphere(m_distance));
		
		// intersect that with the sphere to get a circle:
		if (d*d < 1.0)
		{
			// find two more normals
			n.to_unit();
			assert(fabs(n.abs() - 1.0) < 1e-8);
			
			P3d u(1.0, 0.0, 0.0);
			if (n.x > n.y && n.x > n.z) std::swap(u.x, u.y);
			u -= n * (u*n);
			u.to_unit();
			assert(fabs(u.abs() - 1.0) < 1e-8);
			assert(fabs(u*n) < 1e-8);

			P3d v; cross(v, n, u);
			assert(fabs(v.abs() - 1.0) < 1e-8);
			assert(fabs(v*n) < 1e-8);
			assert(fabs(u*v) < 1e-8);

			double r = sqrt(1.0 - d*d);
			n *= d;
			u *= r;
			v *= r;

			glBegin(GL_LINE_LOOP);
			for (int i = 0, N = 60; i < N; ++i)
			{
				double x, y; sincos(2.0*M_PI*i/N, x, y);
				glVertex3fv((P3f)(n + u * x + v * y));
			}
			glEnd();
		}
		glDisable(GL_LINE_STIPPLE);
		return;
	}

	P3f   n(m_normal);
	float d = m_distance;

	double rxi = 1.0 / axis.range(0);
	float x0 = (float)(-axis.range(0)*rxi), x2 = (float)(axis.range(0)*rxi);
	float y0 = (float)(-axis.range(1)*rxi), y2 = (float)(axis.range(1)*rxi);
	float z0 = (float)(-axis.range(2)*rxi), z2 = (float)(axis.range(2)*rxi);
	
	// intersect with the box edges:

	float p100 = (d - y0*n.y - z0*n.z)/n.x; bool e100 = p100 > x0 && p100 < x2;
	float p102 = (d - y0*n.y - z2*n.z)/n.x; bool e102 = p102 > x0 && p102 < x2;
	float p120 = (d - y2*n.y - z0*n.z)/n.x; bool e120 = p120 > x0 && p120 < x2;
	float p122 = (d - y2*n.y - z2*n.z)/n.x; bool e122 = p122 > x0 && p122 < x2;
	
	float p010 = (d - x0*n.x - z0*n.z)/n.y; bool e010 = p010 > y0 && p010 < y2;
	float p012 = (d - x0*n.x - z2*n.z)/n.y; bool e012 = p012 > y0 && p012 < y2;
	float p210 = (d - x2*n.x - z0*n.z)/n.y; bool e210 = p210 > y0 && p210 < y2;
	float p212 = (d - x2*n.x - z2*n.z)/n.y; bool e212 = p212 > y0 && p212 < y2;

	float p001 = (d - x0*n.x - y0*n.y)/n.z; bool e001 = p001 > z0 && p001 < z2;
	float p021 = (d - x0*n.x - y2*n.y)/n.z; bool e021 = p021 > z0 && p021 < z2;
	float p201 = (d - x2*n.x - y0*n.y)/n.z; bool e201 = p201 > z0 && p201 < z2;
	float p221 = (d - x2*n.x - y2*n.y)/n.z; bool e221 = p221 > z0 && p221 < z2;

	glBegin(GL_LINES);

	// side x = x0
	if (e001 && e010) drawLine(x0, y0, p001, x0, p010, z0);
	if (e021 && e010) drawLine(x0, y2, p021, x0, p010, z0);
	if (e001 && e012) drawLine(x0, y0, p001, x0, p012, z2);
	if (e021 && e012) drawLine(x0, y2, p021, x0, p012, z2);
	if (e001 && e021) drawLine(x0, y0, p001, x0, y2, p021);
	if (e012 && e010) drawLine(x0, p012, z2, x0, p010, z0);

	// side x = x2
	if (e201 && e210) drawLine(x2, y0, p201, x2, p210, z0);
	if (e221 && e210) drawLine(x2, y2, p221, x2, p210, z0);
	if (e201 && e212) drawLine(x2, y0, p201, x2, p212, z2);
	if (e221 && e212) drawLine(x2, y2, p221, x2, p212, z2);
	if (e201 && e221) drawLine(x2, y0, p201, x2, y2, p221);
	if (e212 && e210) drawLine(x2, p212, z2, x2, p210, z0);

	// side y = y0
	if (e001 && e100) drawLine(x0, y0, p001, p100, y0, z0);
	if (e001 && e102) drawLine(x0, y0, p001, p102, y0, z2);
	if (e201 && e100) drawLine(x2, y0, p201, p100, y0, z0);
	if (e201 && e102) drawLine(x2, y0, p201, p102, y0, z2);
	if (e001 && e201) drawLine(x0, y0, p001, x2, y0, p201);
	if (e100 && e102) drawLine(p100, y0, z0, p102, y0, z2);

	// side y = y2
	if (e021 && e120) drawLine(x0, y2, p021, p120, y2, z0);
	if (e021 && e122) drawLine(x0, y2, p021, p122, y2, z2);
	if (e221 && e120) drawLine(x2, y2, p221, p120, y2, z0);
	if (e221 && e122) drawLine(x2, y2, p221, p122, y2, z2);
	if (e021 && e221) drawLine(x0, y2, p021, x2, y2, p221);
	if (e120 && e122) drawLine(p120, y2, z0, p122, y2, z2);

	// side z = z0
	if (e100 && e010) drawLine(p100, y0, z0, x0, p010, z0);
	if (e120 && e010) drawLine(p120, y2, z0, x0, p010, z0);
	if (e100 && e210) drawLine(p100, y0, z0, x2, p210, z0);
	if (e120 && e210) drawLine(p120, y2, z0, x2, p210, z0);
	if (e100 && e120) drawLine(p100, y0, z0, p120, y2, z0);
	if (e010 && e210) drawLine(x0, p010, z0, x2, p210, z0);
	
	// side z = z2
	if (e102 && e012) drawLine(p102, y0, z2, x0, p012, z2);
	if (e122 && e012) drawLine(p122, y2, z2, x0, p012, z2);
	if (e102 && e212) drawLine(p102, y0, z2, x2, p212, z2);
	if (e122 && e212) drawLine(p122, y2, z2, x2, p212, z2);
	if (e102 && e122) drawLine(p102, y0, z2, p122, y2, z2);
	if (e012 && e212) drawLine(x0, p012, z2, x2, p212, z2);
	
	glEnd();
	
	glDisable(GL_LINE_STIPPLE);
}

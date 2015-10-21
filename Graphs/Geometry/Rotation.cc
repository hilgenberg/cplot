#include "Rotation.h"

Rotation::Rotation() : a(0.0, 0.0, 0.0, 1.0)
{
	update();
}
void Rotation::save(Serializer &s) const
{
	s._double(a.x);
	s._double(a.y);
	s._double(a.z);
	s._double(a.w);
}
void Rotation::load(Deserializer &s)
{
	s._double(a.x);
	s._double(a.y);
	s._double(a.z);
	s._double(a.w);
	update();
}

void Rotation::rotate(const P3d &axis, double angle)
{
	a = Quaternion(angle, axis) * a;
	update();
}

void Rotation::rotate(const Rotation &other)
{
	a = other.a * a;
	update();
}

void Rotation::rotate(double psi, double theta, double phi, bool concat)
{
	Quaternion aphi(phi, Z_AXIS), apsi(psi, X_AXIS), atheta(-theta, Y_AXIS);
	Quaternion b = atheta * apsi * aphi;
	a = (concat ? b*a : a*b);
	update();
}

double Rotation::axis_angle(P3d &axis) const
{
	return a.axis_angle(axis);
}

void Rotation::get(double &phi, double &psi, double &theta) const
{
	P3d RX(m.a11, m.a21, m.a31); // m * (1,0,0)
	P3d RY(m.a12, m.a22, m.a32); // m * (0,1,0)
	P3d RZ(m.a13, m.a23, m.a33); // m * (0,0,1)
	
	
	// project RZ onto X-Z, its angle from Z is theta (the first two rotations keep Z in the Y-Z-plane)
	// if the projection is 0, then psi must be -90° or 90°, theta+phi is between RY and Z
	if (fabs(RZ.z) > 1.0 - 1e-8)
	{
		theta = 0.0;
		psi = (RZ.z < 0.0 ? 0.5 : -0.5)*M_PI;
		phi = atan2(RY.x, RY.z);
		return;
	}
	
	theta = atan2(-RZ.x, RZ.z); // RZ.z = -RZ.z can also be done by psi = 180°
	if (fabs(theta) >= 0.5*M_PI)
	{
		theta = -(M_PI-theta);
		if (theta >  M_PI) theta -= 2.0*M_PI;
		if (theta < -M_PI) theta += 2.0*M_PI;
	}
	// undo theta
	double s,c; sincos(-theta, s, c);
	M3d m;
	m.a11 = c; m.a12 = 0; m.a13 = -s;
	m.a21 = 0; m.a22 = 1; m.a23 =  0;
	m.a31 = s; m.a32 = 0; m.a33 =  c;
	
	RX = m*RX;
	RY = m*RY;
	RZ = m*RZ;
	assert(fabs(RZ.x)<1e-12);

	// psi is the angle between Z and RZ in Y-Z
	psi = -atan2(RZ.y, RZ.z);
	
	// undo psi
	sincos(-psi, s, c);
	m.a11 = 1; m.a12 =  0; m.a13 = 0;
	m.a21 = 0; m.a22 =  c; m.a23 = -s;
	m.a31 = 0; m.a32 =  s; m.a33 =  c;
	RX = m*RX;
	
	// phi is the angle between X and RX in X-Y
	phi = atan2(RX.y, RX.x);
}

void Rotation::set(double phi, double psi, double theta)
{
	Quaternion aphi(phi, Z_AXIS), apsi(psi, X_AXIS), atheta(-theta, Y_AXIS);
	a = atheta * apsi * aphi;
	update();
}

void Rotation::update()
{
	a.to_unit();
	m  = a.rotation();
	
	mf = (M3f)m;
	
	mi = m;
	mi.transpose();

#ifdef DEBUG
	M3d mmi = m*mi;
	assert(mmi - M3d(1.0) < 1e-8);
	M3d mii = mi - m.inverse();
	assert(mii < 1e-8);
#endif
}

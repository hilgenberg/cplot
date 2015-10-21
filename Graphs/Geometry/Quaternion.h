#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "AxisIndex.h"
#include "../../Engine/cnum.h" // for sincos

/**
 * Mainly used as unit quaternions to describe rotations.
 * Rotating some vector v with a unit quaternion q works like this:
 * -# Embed it in R^4: v = (vx,vy,vz,0)
 * -# Conjugate with q: r(v) = q v q^-1
 *
 * Quaternions derive from P4<double> and use w for the real part; x,y,z for the imaginary part
 */

class Quaternion : public P4d
{
public:
	Quaternion(double x_, double y_, double z_, double w_) : P4d(x_,y_,z_,w_)
	{
	}
	
	Quaternion operator * (const Quaternion &b) const
	{
		return Quaternion(w * b.x + x * b.w + y * b.z - z * b.y,
					w * b.y - x * b.z + y * b.w + z * b.x,
					w * b.z + x * b.y - y * b.x + z * b.w,
					w * b.w - x * b.x - y * b.y - z * b.z);
	}
	
	Quaternion &conjugate()
	{
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}
	
	Quaternion operator - () const
	{
		return Quaternion(-x, -y, -z, -w);
	}
	
	Quaternion &invert() // multiplicative inverse
	{
		double q = -1.0 / absq();
		w *= -q;
		x *=  q;
		y *=  q;
		z *=  q;
		return *this;
	}
	
	Quaternion &to_unit()
	{
		P4d::to_unit();
		if (!defined(x) || !defined(y) || !defined(z) || !defined(w))
		{
			x = y = z = 0.0; w = 1.0;
		}
		return *this;
	}
	
	Quaternion exp() const
	{
		double e = std::exp(w);
		double m = x*x + y*y + z*z;
		double s, c;
		sincos(m, s, c);
		s *= e; c *= e;
		return Quaternion(s*x, s*y, s*z, c*w);
	}

	//--- to and from axis angle ---------------------------------------------------------------------------------------

	Quaternion(double angle, AxisIndex axis)
	{
		switch(axis)
		{
			case X_AXIS: sincos(0.5*angle, x, w); y = z = 0.0; break;
			case Y_AXIS: sincos(0.5*angle, y, w); x = z = 0.0; break;
			case Z_AXIS: sincos(0.5*angle, z, w); x = y = 0.0; break;
		}
	}

	Quaternion(double angle, const P3d &axis)
	{
		double r = axis.abs();
		double s;
		sincos(0.5*angle, s, w);
		x = axis.x * s / r;
		y = axis.y * s / r;
		z = axis.z * s / r;
	}
	
	double axis_angle(P3d &axis) const
	{
		//if (a.w > 1.0 || a.w < -1.0) normalize();
		double f = 1.0 / abs();
		double wf = w*f;
		double s = sqrt(1.0 - wf*wf);
		double angle = 2.0 * acos(wf);
		
		if (s < 1e-12)
		{
			// if s is zero, the direction is not important
			axis.x = 1.0;
			axis.y = 0.0;
			axis.z = 0.0;
		}
		else
		{
			f /= s;
			axis.x = x*f;
			axis.y = y*f;
			axis.z = z*f;
		}
		return angle;
	}

	//--- to and from rotation matrix ----------------------------------------------------------------------------------
	
	M3d rotation() const /// Quaternion to rotation matrix
	{
		// works for non-normalized ones too
		double Nq = absq();
		double  s = (Nq > 0.0) ? 2.0 / Nq : 0.0;
		double  X = x*s,  Y = y*s,  Z = z*s;
		double wX = w*X, wY = w*Y, wZ = w*Z;
		double xX = x*X, xY = x*Y, xZ = x*Z;
		double yY = y*Y, yZ = y*Z, zZ = z*Z;
		
		M3d m;
		m.a11 = 1.0-(yY+zZ); m.a12 =      xY-wZ;  m.a13 =      xZ+wY;
		m.a21 =      xY+wZ;  m.a22 = 1.0-(xX+zZ); m.a23 =      yZ-wX;
		m.a31 =      xZ-wY;  m.a32 =      yZ+wX;  m.a33 = 1.0-(xX+yY);
		return m;
	}
	Quaternion(const M3d &m) /// Rotation matrix to quaternion
	:
	P4d(copysign(0.5*sqrt(1.0+m.a11-m.a22-m.a33), m.a32-m.a23),
	    copysign(0.5*sqrt(1.0-m.a11+m.a22-m.a33), m.a13-m.a31),
	    copysign(0.5*sqrt(1.0-m.a11-m.a22+m.a33), m.a21-m.a12),
	    0.5*sqrt(1.0 + m.a11 + m.a22 + m.a33) )
	{
	}
};

inline double dotproduct(const Quaternion &a, const Quaternion &b)
{
	return a.P4d::operator* (b);
}


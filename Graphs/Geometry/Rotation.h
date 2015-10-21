#pragma once

#include "Matrix.h"
#include "Quaternion.h"
#include "../../Persistence/Serializer.h"

class Rotation : public Serializable
{
public:
	Rotation(); // starts as identity
	
	void save(Serializer &s) const;
	void load(Deserializer &s);

	void rotate(double rx, double ry, double rz, bool concat=true);
	void rotate(const Rotation &other);
	void rotate(const P3d &axis, double angle);
	
	P3d operator() (const P3d &v) const{ return m  * v; } ///< Image of v (double version)
	P3f operator() (const P3f &v) const{ return mf * v; } ///< Image of v (float version)
	P3d operator[] (const P3d &v) const{ return mi * v; } ///< Preimage of v

	double axis_angle(P3d &axis) const; ///< Get axis-angle representation

	void set(double  phi, double  psi, double  theta); ///< Set from Euler angles
	void get(double &phi, double &psi, double &theta) const; ///< Get Euler angles

private:
	Quaternion a; ///< Primary storage
	
	mutable M3d  m; ///< a as a rotation matrix
	mutable M3d mi; ///< m transposed/inverted
	mutable M3f mf; ///< mf = (M3f)m

	void update(); ///< Sync m, mi and mf to new value of a
};

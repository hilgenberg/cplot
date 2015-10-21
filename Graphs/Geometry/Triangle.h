#pragma once
#include "Vector.h"

class Triangle
{
public:
	Triangle(const P2d &A, const P2d &B, const P2d &C) : A(A), B(B), C(C)
	{
		m.a11 = A.x-C.x; m.a12 = B.x-C.x;
		m.a21 = A.y-C.y; m.a22 = B.y-C.y;
		D = m.det();
	}
	inline bool contains(double x, double y)
	{
		// aA+bB+cC = (x,y), a+b+c = 1 -->
		// x - C.x = a*(A.x-C.x) + b*(B.x-C.x);
		// y - C.y = a*(A.y-C.y) + b*(B.y-C.y);
		// --> a,b in [0..1]
		double Da = (x-C.x)*m.a22 - (y-C.y)*m.a12;
		double Db = (y-C.y)*m.a11 - (x-C.x)*m.a21;
		return D >= 0.0 ? (Da >= 0.0 && Db >= 0.0 && Da+Db <= D) : (Da <= 0.0 && Db <= 0.0 && Da+Db >= D);
	}
private:
	P2d A, B, C;
	M2d m;
	double D;
};

#include "Functions.h"

#include <cmath>
#include <cassert>

double identity(double z){ return z; }
void   identity(const cnum &z, cnum &r){ r = z; }

double zero(double        ){ return 0.0; }
double zero(double, double){ return 0.0; }

void sqrt_(const cnum &z, cnum &r){ r = std::sqrt(z); }
void sqrt_(double z, cnum &r)
{
	if(z >= 0.0)
	{
		r = ::sqrt(z);
	}
	else
	{
		r.real(0.0);
		r.imag(::sqrt(-z));
	}
}
double rsqrt(double x)
{
	return x >= 0.0 ? ::sqrt(x) : UNDEFINED;
}

void  sqr(const cnum &z, cnum &r){ r = z; r *= z; }
void cube(const cnum &z, cnum &r){ r = z; r *= z; r *= z; }
void pow4(const cnum &z, cnum &r){  sqr(z, r); sqr(r, r); }
void pow5(const cnum &z, cnum &r){ pow4(z, r); r = z * r; }
void pow6(const cnum &z, cnum &r){ pow4(z, r); r = z * z * r; }
void pow7(const cnum &z, cnum &r){ pow4(z, r); r = z * z * z * r; }
void pow8(const cnum &z, cnum &r){  sqr(z, r); sqr(r, r); sqr(r, r); }
void pow9(const cnum &z, cnum &r){ pow8(z, r); r = z * r; }
double  sqr(double z){ return z * z; }
double cube(double z){ return z * z * z; }
double pow4(double z){ return sqr(sqr(z)); }
double pow5(double z){ return z * sqr(sqr(z)); }
double pow6(double z){ return z * z * sqr(sqr(z)); }
double pow7(double z){ return z * z * z * sqr(sqr(z)); }
double pow8(double z){ return sqr(sqr(sqr(z))); }
double pow9(double z){ return z * sqr(sqr(sqr(z))); }

void  c_abs(const cnum &z, cnum &r){ r = ::abs(z); }
void c_absq(const cnum &z, cnum &r){ r = std::norm(z); }
void  c_arg(const cnum &z, cnum &r){ r = std::arg(z); }
void  c_sgn(const cnum &z, cnum &r){ if (!isz(z)) r = z / abs(z); }
void   c_re(const cnum &z, cnum &r){ r = z.real(); }
void   c_im(const cnum &z, cnum &r){ r = z.imag(); }
double  abs(const cnum &z){ return std::abs(z); }
double  arg(const cnum &z){ return std::arg(z); }
double  sgn(double      z){ return z < 0.0 ? -1.0 : z > 0.0 ? 1.0 : 0.0; }
double   re(const cnum &z){ return z.real(); }
double   im(const cnum &z){ return z.imag(); }

double arg2(double x, double y){ return std::arg(cnum(x,y)); }
void   arg2(const cnum &x, const cnum &y, cnum &r){ r = std::arg(cnum(x.real()-y.imag(), x.imag()+y.real())); }

double fowler_angle(const cnum &z)
{
	double ax = abs(z.real()), ay = abs(z.imag());
	int sector = (ax < ay) ? 1 : 0;
	if (z.real() < 0)  sector |= 2;
	if (z.imag() < 0)  sector |= 4;
	
	switch (sector)
	{
		case 0: return ax==0.0 ? 0.0 : ay/ax; // [  0, 45]
		case 1: return 2.0 - ax/ay;      // ( 45, 90]
		case 3: return 2.0 + ax/ay;      // ( 90,135)
		case 2: return 4.0 - ay/ax;      // [135,180]
		case 6: return 4.0 + ay/ax;      // (180,225]
		case 7: return 6.0 - ax/ay;      // (225,270)
		case 5: return 6.0 + ax/ay;      // [270,315)
		case 4: return 8.0 - ay/ax;      // [315,360)
	}
	assert(false);
	return UNDEFINED;
}
double fowler_angle(double x)
{
	return x < 0.0 ? 4.0 : 0.0;
}
void fowler_angle(const cnum &z, cnum &r)
{
	r.real(fowler_angle(z));
	r.imag(0.0);
}

void    swap(const cnum &z, cnum &r){ r.real(z.imag()); r.imag(z.real()); }
void    conj(const cnum &z, cnum &r){ r = std::conj(z); }
void  round_(const cnum &z, cnum &r){ r.real(std::round(z.real())); r.imag(std::round(z.imag())); }
void   floor(const cnum &z, cnum &r){ r.real(std::floor(z.real())); r.imag(std::floor(z.imag())); }
void    ceil(const cnum &z, cnum &r){ r.real(std::ceil (z.real())); r.imag(std::ceil (z.imag())); }
void    c_sp(const cnum &x, const cnum &y, cnum &r){ r = x.real()*y.real() + x.imag()*y.imag(); }
void   c_det(const cnum &x, const cnum &y, cnum &r){ r = x.real()*y.imag() - x.imag()*y.real(); }
void  c_dist(const cnum &x, const cnum &y, cnum &r){ c_abs(x-y, r); }
void   hypot(const cnum &x, const cnum &y, cnum &r){ sqrt_(x*x+y*y, r);}
double round_(double z){ return std::floor(z+0.5); }
double    sp(const cnum &x, const cnum &y){ return x.real()*y.real() + x.imag()*y.imag(); }
double   det(const cnum &x, const cnum &y){ return x.real()*y.imag() - x.imag()*y.real(); }
double  dist(const cnum &x, const cnum &y){ return ::abs(x-y); }
double  dist(double      x, double      y){ return  fabs(x-y); }

void mida(const cnum &x, const cnum &y, cnum &r){ r = 0.5 * (x+y);}
void midg(const cnum &x, const cnum &y, cnum &r){ sqrt_(x*y, r); }
void midh(const cnum &x, const cnum &y, cnum &r){ r = 2.0 * x * y / (x+y); }
double mida(double x, double y){ return 0.5 * (x+y); }
double midg(double x, double y){ return sqrt(x*y); }
double midh(double x, double y){ return 2.0 * x * y / (x+y); }

void cblend_(const cnum &a, const cnum &b, const cnum &T, cnum &ret)
{
	double t = 0.5 * (1.0 - cos(T.real()*M_PI)); // 0 -> 1 for t: 0 -> 1
	ret = (1.0-t) * a + t * b;
}
void blend_(const cnum &a, const cnum &b, const cnum &T, cnum &ret)
{
	double t = std::min(std::max(T.real(), 0.0), 1.0); // clamp to [0,1]
	ret = (1.0-t) * a + t * b;
}
void mix_(const cnum &a, const cnum &b, const cnum &t, cnum &ret)
{
	ret = (1.0-t) * a + t * b;
}

double clamp (double x){ return x < 0.0 ? 0.0 : x > 1.0 ? 1.0 : x; }
double clamps(double x, double a, double b)
{
	if (a < b) return x <= a ? a : x >= b ? b : x;
	return x <= b ? b : x >= a ? a : x;
}
void clamp (const cnum &x, cnum &ret)
{
	ret.real(clamp(x.real()));
	ret.imag(clamp(x.imag()));
}
void clamps(const cnum &x, const cnum &a, const cnum &b, cnum &ret)
{
	ret.real(clamps(x.real(), a.real(), b.real()));
	ret.imag(clamps(x.imag(), a.imag(), b.imag()));
}

void qtest(const cnum &a, const cnum &b, const cnum &c, const cnum &d, cnum &ret){ ret = a + b + c + d; }


#include "Functions.h"
#include <complex>

using std::exp;
using std::cos;
using std::sin;
using std::log;

//--- utility ----------------------------------------------------------------------------------------------------------

static inline void muli(cnum &z) // z *= i
{
	double tmp = z.real();
	z.real(-z.imag());
	z.imag(tmp);
}
static inline void muli(const cnum &z, cnum &r) // r = i*z
{
	r.real(-z.imag());
	r.imag( z.real());
}
static inline void divi(cnum &z) // z /= i
{
	double tmp = z.real();
	z.real(z.imag());
	z.imag(-tmp);
}

//--- exp, log, pow ----------------------------------------------------------------------------------------------------

void  exp_(const cnum &x, cnum &r){ r = std::exp(x); }
void  log_(const cnum &x, cnum &r){ r = std::log(x); }
void log10(const cnum &x, cnum &r){ r = std::log10(x); }
void  log2(const cnum &x, cnum &r){ r = std::log(x); r /= M_LN2; }

void  cpow(const cnum &x, const cnum &y, cnum &r){ r = std::pow(x, y); }
void   pow(const cnum &x, double      y, cnum &r){ r = std::pow(x, y); }
void   pow(const cnum &x, unsigned    n, cnum &r)
{
	cnum p(x);
	r = 1.0;
	while(n)
	{
		if (n & 1) r *= p;
		p  *= p;
		n >>= 1;
	}
}
void rcpow(double x, double y, cnum &r)
{
	if (x > 0.0 || y-floor(y) < EPSILON)
	{
		r.real(pow(x, y));
		r.imag(0.0);
	}
	else
	{
		r = std::pow(cnum(x), y);
	}
}

void spow(const cnum &x, const cnum &y, cnum &r)
{
	double rx = abs(x);
	r = std::pow(rx, y);
	r *= x/rx;
}
double spow(double x, double y)
{
	double r = std::pow(fabs(x), y);
	return copysign(r, x);
}

//--- trig, hyp, etc ---------------------------------------------------------------------------------------------------

void cos(const cnum &v, cnum &r){ r = std::cos(v); }
void sin(const cnum &v, cnum &r){ r = std::sin(v); }
void tan(const cnum &v, cnum &r){ r = std::tan(v); }
void cot(const cnum &v, cnum &r){ r = 1.0 / std::tan(v); }
void sec(const cnum &v, cnum &r){ r = 1.0 / std::cos(v); }
void csc(const cnum &v, cnum &r){ r = 1.0 / std::sin(v); }
double cot(double v){ return 1.0 / tan(v); }
double sec(double v){ return 1.0 / cos(v); }
double csc(double v){ return 1.0 / sin(v); }

void cosh(const cnum &v, cnum &r){ r = std::cosh(v); }
void sinh(const cnum &v, cnum &r){ r = std::sinh(v); }
void tanh(const cnum &v, cnum &r){ r = std::tanh(v); }
void coth(const cnum &v, cnum &r){ r = 1.0 / std::tanh(v); }
void sech(const cnum &v, cnum &r){ r = 1.0 / std::cosh(v); }
void csch(const cnum &v, cnum &r){ r = 1.0 / std::sinh(v); }

double coth(double v){ return 1.0 / std::tanh(v); }
double sech(double v){ return 1.0 / std::cosh(v); }
double csch(double v){ return 1.0 / std::sinh(v); }

void acos(const cnum &z, cnum &r)
{
	//r = std::acos(z);
	sqrt_(z*z-1.0, r); r += z; log_(r, r); divi(r);
}
void asin(const cnum &z, cnum &r)
{
	sqrt_(1.0-z*z, r);
	cnum t; muli(z, t);
	r += t;
	log_(r, r); divi(r);
}
void atan(const cnum &z, cnum &r)
{
	muli(z,r);
	log_( (1.0-r) / (1.0+r), r);
	muli(r); r *= 0.5;
}

void acot(const cnum &z, cnum &r)
{
	muli(z,r);
	log_( (1.0+r) / (r-1.0), r);
	muli(r); r *= 0.5;
}
void asec(const cnum &z, cnum &r)
{
	sqrt_(1.0-z*z, r);
	r += 1.0;
	r /= z;
	log_(r, r);
	divi(r);
}
void acsc(const cnum &z, cnum &r)
{
	sqrt_(z*z-1.0, r);
	r.imag(r.imag()+1.0);
	r /= z;
	log_(r, r);
	divi(r);
}

void asinh(const cnum &z, cnum &r){ sqrt_(z*z+1.0, r); r+=z; log_(r, r); }
void acosh(const cnum &z, cnum &r){ sqrt_(z*z-1.0, r); r+=z; log_(r, r); }
void atanh(const cnum &z, cnum &r){ log_((z+1.0)/(1.0-z), r); r*=0.5; }
void acoth(const cnum &z, cnum &r){ log_((z+1.0)/(z-1.0), r); r*=0.5; }
void acsch(const cnum &z, cnum &r){ sqrt_(z*z+1.0, r); r+=1.0; r/=z; log_(r, r); }
void asech(const cnum &z, cnum &r){ sqrt_(1.0-z*z, r); r+=1.0; r/=z; log_(r, r); }

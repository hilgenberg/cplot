#include "Functions.h"

void  add(const cnum &z, const cnum &w, cnum &ret){ ret = z; ret += w; }
void  sub(const cnum &z, const cnum &w, cnum &ret){ ret = z; ret -= w; }
void  mul(const cnum &z, const cnum &w, cnum &ret){ ret = z; ret *= w; }
void cdiv(const cnum &z, const cnum &w, cnum &ret){ ret = z; ret /= w; }
double  add(double z, double w){ return z + w; }
double  sub(double z, double w){ return z - w; }
double  mul(double z, double w){ return z * w; }
double  div(double z, double w){ return z / w; }

static inline double FMOD(double x,double y){ return x - y*floor(x/y); }

void cmod(const cnum &x, const cnum &y, cnum &ret)
{
	ret = (is_real(x) && is_real(y)) ? FMOD(x.real(), y.real()) : UNDEFINED;
}
double crmod(const cnum &z, const cnum &w)
{
	return (is_real(z) && is_real(w)) ? FMOD(z.real(), w.real()) : UNDEFINED;
}
double mod(double z, double w){ return FMOD(z,w); }

void negate(const cnum &z, cnum &ret){ ret = -z; }
void invert(const cnum &z, cnum &ret){ double r = absq(z); ret.real(z.real()/r); ret.imag(-z.imag()/r); }

double negate(double z){return -z;    }
double invert(double z){return 1.0/z; }

void construct(double re, double im, cnum &r){ r.real(re); r.imag(im); }
void construct(const cnum &a, const cnum &b, cnum &r) // a + ib
{
	r.real(a.real() - b.imag());
	r.imag(a.imag() + b.real());
}

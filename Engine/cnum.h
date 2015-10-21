#pragma once

#include <complex>
#include <limits>
#include <cmath>

class Namespace;

enum
{
	R_Complex     =  0,                 // any complex number
	R_Real        =  1,                 // any real number
	R_Imag        =  2,                 // pure imaginary
	R_Unit        =  4,                 // |z| <= 1
	R_Interval    =  R_Unit + R_Real,   // |x| <= 1
	R_Integer     =  8 + R_Real,        // any integer
	R_NonNegative = 16 + R_Real,        // x >= 0
	R_Positive    = 32 + R_NonNegative, // x > 0
	R_Zero        = R_Integer | R_NonNegative | R_Imag | R_Unit,
	R_One         = R_Integer | R_Positive | R_Unit
};
typedef int Range;

inline bool subset(Range sub, Range all)
{
	return !(~sub & all);
}

enum PrintingStyle
{
	PS_Debug,       // +((*)(x,y), sin(z))
	PS_Function,    // add(mul(x,y),sin(z))
	PS_Console,     // x*y+sin(z)
	PS_Input,       // guaranteed to parse to the printed object
	PS_HTML,        // DS_Console with HTML superscripts and < --> &lt;, etc.
	PS_LaTeX,       // x\cdoty+\sin(z)
	PS_Mathematica, //x*y+Sin[z]
	PS_Head         // + (and ! --> •!)
};

/**
 * @defgroup cnum Complex Numbers
 * @{
 */

#define UNDEFINED std::numeric_limits<double>::quiet_NaN()
#define EPSILON   std::numeric_limits<double>::epsilon()

/**
 * Complex numbers.
 */
typedef std::complex<double> cnum;

inline bool defined(double      x){ return std::isfinite(x); } ///< @return true if not NAN or INF
inline bool defined(const cnum &z){ return ::defined(z.real()) && ::defined(z.imag()); } ///< @return true if neither real nor imag part is NAN or INF

using std::abs;
using std::arg;
inline double absq(const cnum &z){ return std::norm(z); } ///< absq(z) = re(z)^2 + im(z)^2

/// Zero test with EPSILON precision
inline bool     isz(const cnum &z){ return fabs(z.real()) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool     isz(double x){ return fabs(x) < EPSILON; }

/// Unit test with EPSILON precision
inline bool is_one(const cnum &z){ return fabs(z.real()-1.0) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool is_one(double x){ return fabs(x-1.0) < EPSILON; }
inline bool is_minusone(const cnum &z){ return fabs(z.real()+1.0) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool is_minusone(double x){ return fabs(x+1.0) < EPSILON; }

/// Realness test with EPSILON precision
inline bool is_real(const cnum &z){ return fabs(z.imag()) < EPSILON && ::defined(z.real()); }

/// Test for being pure imaginary with EPSILON precision
inline bool is_imag(const cnum &z){ return fabs(z.real()) < EPSILON && ::defined(z.imag()); }

/// Integer test with EPSILON precision
inline bool is_int(const cnum &z){ return fabs(z.real() - round(z.real())) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool is_int(double      x){ return fabs(x        - round(x))        < EPSILON; }

/// Rounding to int
inline int  to_int(const cnum  &n){ return (int)round(n.real()); }
inline int  to_int(double       n){ return (int)round(n);        }

/// Naturalness test (integer and >= 0) with EPSILON precision
inline bool is_natural(const cnum &n){ return is_int(n) && n.real() > -0.1; }
inline bool is_natural(double      n){ return is_int(n) && n        > -0.1; }

/// Rounding to unsigned
inline unsigned to_natural(const cnum  &n){ return (unsigned)round(n.real()); }
inline unsigned to_natural(double       n){ return (unsigned)round(n);        }

inline Range range(const cnum &z)
{
	if (isz(z)) return R_Zero;
	Range r = R_Complex;
	if (is_real(z))
	{
		r = R_Real;
		if (is_int(z.real())) r |= R_Integer;
		if (fabs(z.real()) <= 1.0 + EPSILON) r |= R_Unit;
		if (z.real() > 0.0) r |= R_Positive;
		if (z.real() >= -EPSILON) r |= R_NonNegative;
	}
	else if (is_imag(z))
	{
		r = R_Imag;
		if (fabs(z.imag()) <= 1.0 + EPSILON) r |= R_Unit;
	}
	else
	{
		if (absq(z) <= 1.0 + EPSILON) r |= R_Unit;
	}
	return r;
}

inline int uglyness(const cnum &z)
{
	if (is_int(z.real()) && is_int(z.imag())) return 0;
	if (is_int(4.0*z.real()) && is_int(4.0*z.imag())) return 1;
	return 2;
}

#if 0 // todo
bool cancel(cnum   &a, cnum   &b); // cancel common integer parts
bool cancel(double &a, double &b);
#endif

/// Equality test with EPSILON precision
inline bool eq(const cnum &z, const cnum &w)
{
	return fabs(z.real()-w.real()) < EPSILON && fabs(z.imag()-w.imag()) < EPSILON;
}
inline bool eq(double z, const cnum &w){ return fabs(z-w.real()) < EPSILON && fabs(w.imag()) < EPSILON; }
inline bool eq(const cnum &z, double w){ return fabs(z.real()-w) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool eq(double x, double y){ return fabs(x-y) < EPSILON; }

inline void to_unit(cnum &z){ z /= abs(z); } ///< Divides z by its absolute value.

inline void invert(cnum &z) /// Set z to 1/z
{
	double rq = std::norm(z);
	if (rq == 0.0)
	{
		z = UNDEFINED;
		return;
	}
	z /= rq;
	z.imag(-z.imag());
}
inline cnum inverse(const cnum &z) ///< @return 1/z
{
	double rq = std::norm(z);
	return rq == 0.0 ? cnum(UNDEFINED) : cnum(z.real()/rq, -z.imag()/rq);
}

/// Print the number numerically, i.e. like 1.23+4.56i
std::ostream & operator<< (std::ostream &out, const cnum &z);

/// Print the number numerically, i.e. like 1.23+4.56i
std::string to_string(const cnum &z, PrintingStyle ds = PS_Console);

/// Print the number symbolically, if the value has a name in the Namespace, otherwise numerically.
std::string to_string(const cnum &z, const Namespace &ns, PrintingStyle ds = PS_Console);

bool prints_sign(const cnum &z); // -x, -x+iy, -iy, +INF, ...
bool prints_sum(const cnum &z);  // x+iy, x-iy

/**
 * Parses and evaluates a string.
 * @param s  The string to evaluate. Must evaluate to a constant (as opposed to function).
 * @param ns Contains all symbols that can be used.
 * @return The parsed value or UNDEFINED on error.
 * @throw  Possibly std::bad_alloc.
 */
cnum evaluate(const std::string &s, const Namespace &ns);

cnum evaluate(const std::string &s); // in default RootNamespace

/** @} */

inline void sincos(double angle, double &sin_value, double &cos_value)
{
	asm("fsincos" : "=t" (cos_value), "=u" (sin_value) : "0" (angle));
	//cos_value = cos(angle); sin_value = sin(angle)
}


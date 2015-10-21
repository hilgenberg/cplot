#include "../cnum.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"

#include <boost/math/special_functions/bessel.hpp>
#include <boost/math/special_functions/expint.hpp>
#include <boost/math/special_functions/airy.hpp>

#pragma GCC diagnostic pop

using namespace boost::math::policies;

typedef policy<
domain_error    <ignore_error>,
overflow_error  <ignore_error>,
underflow_error <ignore_error>,
denorm_error    <ignore_error>,
pole_error      <ignore_error>,
evaluation_error<ignore_error>,
digits10<8>
> PREC;

//----------------------------------------------------------------------------------------------------------------------
// Bessel functions
//----------------------------------------------------------------------------------------------------------------------

#define BESSEL(NAME, FUNC) \
void NAME(const cnum &n, const cnum &z, cnum &r){ try{\
		r = (!is_real(z) || !is_real(n)) ? UNDEFINED\
		: is_natural(n) ? boost::math::FUNC(to_int(n), z.real(), PREC())\
		: boost::math::FUNC(n.real(), z.real(), PREC());\
	}catch(...){ r = UNDEFINED; }}\
\
double NAME(const cnum &n, const cnum &z){ try{\
		return (!is_real(z) || !is_real(n)) ? UNDEFINED\
		: is_natural(n) ? boost::math::FUNC(to_int(n), z.real(), PREC())\
		: boost::math::FUNC(n.real(), z.real(), PREC());\
	}catch(...){ return UNDEFINED; }}\
\
double NAME(double n, double z){ try{\
		return is_natural(n) ? boost::math::FUNC(to_int(n), z, PREC())\
		: boost::math::FUNC(n, z, PREC());\
	}catch(...){ return UNDEFINED; }}

BESSEL(bessel_J, cyl_bessel_j)
BESSEL(bessel_Y, cyl_neumann)
BESSEL(bessel_I, cyl_bessel_i)
BESSEL(bessel_K, cyl_bessel_k)

#undef BESSEL

//----------------------------------------------------------------------------------------------------------------------
// Elliptic integral Ei
//----------------------------------------------------------------------------------------------------------------------

void expint_i(const cnum &z, cnum &r)
{
	try
	{
		r = is_real(z) ? boost::math::expint(z.real(), PREC()) : UNDEFINED;
	}
	catch(...)
	{
		r = UNDEFINED;
	}
}
double expint_i(const cnum &z)
{
	try
	{
		return is_real(z) ? boost::math::expint(z.real(), PREC()) : UNDEFINED;
	}
	catch(...)
	{
		return UNDEFINED;
	}
}
double expint_i(double z)
{
	try
	{
		return boost::math::expint(z, PREC());
	}
	catch(...)
	{
		return UNDEFINED;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Elliptic integral En
//----------------------------------------------------------------------------------------------------------------------

void expint_n(const cnum &n, const cnum &z, cnum &r)
{
	try
	{
		r = (is_real(z) && is_natural(n)) ? boost::math::expint(to_natural(n), z.real(), PREC()) : UNDEFINED;
	}
	catch(...)
	{
		r = UNDEFINED;
	}
}
double expint_n(const cnum &n, const cnum &z)
{
	try
	{
		return (is_real(z) && is_natural(n)) ? boost::math::expint(to_natural(n), z.real(), PREC()) : UNDEFINED;
	}
	catch(...)
	{
		return UNDEFINED;
	}
}
double expint_n(double n, double z)
{
	try
	{
		return (is_natural(n)) ? boost::math::expint(to_natural(n), z, PREC()) : UNDEFINED;
	}
	catch(...)
	{
		return UNDEFINED;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Airy Ai and Bi, Ai' and Bi'
//----------------------------------------------------------------------------------------------------------------------

#define AIRY(NAME, FUNC) \
void NAME(const cnum &z, cnum &r){ try{\
	r = is_real(z) ? boost::math::FUNC(z.real(), PREC()) : UNDEFINED;\
	}catch(...){ r = UNDEFINED; }}\
\
double NAME(const cnum &z){ try{\
	return is_real(z) ? boost::math::FUNC(z.real(), PREC()) : UNDEFINED;\
	}catch(...){ return UNDEFINED; }}\
\
double NAME(double z){ try{\
	return boost::math::FUNC(z, PREC());\
	}catch(...){ return UNDEFINED; }}

AIRY(airy_ai, airy_ai)
AIRY(airy_bi, airy_bi)
AIRY(airy_ai_prime, airy_ai_prime)
AIRY(airy_bi_prime, airy_bi_prime)

#undef AIRY

#include "../WorkingTree.h"
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree::simplify_powers
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::simplify_powers(bool &change)
{
	//------------------------------------------------------------------------------------------------------------------
	// simplify powers
	//------------------------------------------------------------------------------------------------------------------

	for (auto &c : children) c.simplify_powers(change);
	
	if (!is_operator(ns().Pow)) return;
	
	double x; WorkingTree &z  = child(0); if (! z.is_real(x)) x = UNDEFINED;
	double b; WorkingTree &bx = child(1); if (!bx.is_real(b)) b = UNDEFINED;
	
	// 1^z = z^0 = 1
	if (defined(x) && ::is_one(x) || defined(b) && isz(b))
	{
		*this = 1.0;
		change = true;
		return;
	}
	
	// 0^n = 0 for n > 0, NAN for n < 0
	if (defined(x) && isz(x) && defined(b))
	{
		*this = (b > 0.0 ? 0.0 : UNDEFINED);
		change = true;
		return;
	}

	if (defined(x) && eq(x, M_E) && (bx.is_function(ns().Log) || bx.is_function(ns().Log_)))
	{
		// e^log(z) = z
		pull(1,0);
		change = true;
		return;
	}

	//------------------------------------------------------------------------------------------------------------------
	// (1/z)^b --> 1 / z^b
	//------------------------------------------------------------------------------------------------------------------
	// z^w = exp(w lnz), lnz = ln|z| + i*argz
	// (1/z)^b = exp(b ln(1/z))
	// 1/(z^b) = 1/exp(b lnz) = exp(-b lnz)
	// equal if b*(ln(1/z)+ln(z)) = 2kπi
	// lnz = ln|z| + i*argz, so
	// b*(ln|1/z| + iargz + ln|z| + iarg(1/z)) = i*b*(argz + arg(1/z)) = i*b*(0 or 2π)
	// This works if b is integer or z is not on the negative real axis.
	
	if (z.is_inversion() && (defined(b) && is_int(b))) // todo: check z for being positive and real at least
	{
		z.pull(0);
		flip_involution(ns().Invert);
		change = true;
		return;
	}

	//------------------------------------------------------------------------------------------------------------------
	// (z^a)^b
	//------------------------------------------------------------------------------------------------------------------
	if (z.is_operator(ns().Pow))
	{
		double a; WorkingTree &ax = z.child(1); if (!ax.is_real(a)) a = UNDEFINED;
		
		// z^w = exp(w lnz), lnz = ln|z| + i*argz
		// exp(log(z)) = z, log(exp(z)) = rez + i*µ(imz)  for µ(x) = fmod(x, [-π,π])
		// for real x: lnx = ln|x| + iπ*(x<0), exp(log(x)) = log(exp(x)) = x
		
		// => (z^a)^b = exp(b ln(z^a)) = exp(b ln(exp(a lnz)))
		//            = exp(b [ln|exp(a lnz)| + i*arg(exp(a lnz))])
		//            = exp(b [re(a lnz) + i*µ(im(a lnz))])
		//            = exp(b ( re(a)*ln|z| - im(a)*argz + i*µ( re(a)*argz + im(a)*ln|z|) ))
		
		// z^(ab) = exp(ab lnz) and (z^a)^b / z^(ab) =!= 1
		// <=> 2kπi = b*re(a lnz) + i*b*µ(im(a lnz)) - ab lnz
		//          = b*re(a lnz) + i*b*µ(im(a lnz)) - b*(re(a lnz)+i*im(a lnz))
		//          = i*b*(µ(im(a lnz)) - im(a lnz))
		// <=>  2kπ = b*(µ(im(a lnz)) - im(a lnz))
		
		// if a is real, then
		// b*(µ(im(a lnz)) - im(a lnz)) = b*(µ(a*im(lnz)) - a*im(lnz)) = b*(µ(a*argz) - a*argz)
	
		// if a is real and |a| <= 1
		// => b*(µ(a*argz) - a*argz) = b*0 = 0
		
		// [Case I] equal for real a with |a| <= 1 and any b
		if (defined(a) && fabs(a) <= 1.0)
		{
			ax *= std::move(bx);
			pull(0);
			change = true;
			return;
		}

		// if a is real and b is integer:
		//  => 2kπ  = b*(µ(a*argz) - a*argz)
		// <=> 2k'π = µ(a*argz) - a*argz = µ(x)-x = 2k''π because µ is fmod
	
		// [Case II] equal for a real and b integer
		if (defined(b) && is_int(b) && (defined(a) || ax.is_real()))
		{
			ax *= std::move(bx);
			pull(0);
			change = true;
			return;
		}
			
		// if a is real and z > 0:
		// => b*(µ(a*argz) - a*argz) = b*(µ(a*0) - a*0) = 0
		// [Case III] equal for a real and z > 0 (not implemented though)
	
		//--------------------------------------------------------------------------------------------------------------
		// some special cases
		//--------------------------------------------------------------------------------------------------------------

		if (defined(a) && defined(b) && z.child(0).is_real()) // a and b are real constants, z is real
		{
			double zz; WorkingTree &zzx = z.child(0); if (!zzx.is_real(zz)) zz = UNDEFINED;

			if (eq(b, 0.5) && is_int(a) && // (z^k) ^ 1/2
				a > std::numeric_limits<int>::min() && a < std::numeric_limits<int>::max())
			{
				int k = (int)std::round(a);
				
				if (k == 2) // √(x^2) = |x|
				{
					pull(0, 0);
					pack(ns().Abs);
					change = true;
					return;
				}
				
				if (abs(k) % 4 == 0) // √(x^4n) = x^2n
				{
					pull(0);
					child(1) = k/2;
					change = true;
					return;
				}
				
				if (abs(k) % 2 == 0) // √(x^2n) = |x|^n
				{
					child(1) = k/2;
					child(0).type = TT_Function;
					child(0).function = ns().Abs;
					child(0).remove_child(1);
					change = true;
					return;
				}
			}
		}
	} // end of nested powers case

	// z^k
	if (defined(b) && is_int(b) && b > std::numeric_limits<int>::min() && b < std::numeric_limits<int>::max())
	{
		int k = (int)std::round(b);
		bool even = !(abs(k) & 1);
		
		if (k == 1) // z^1 = z
		{
			pull(0);
			change = true;
			return;
		}
		
		if (k == -1) // z^-1 = 1/z
		{
			operator_ = ns().Invert;
			remove_child(1);
			change = true;
			return;
		}
		
		if (z.is_negation()) // (-z)^2k = z^2k, (-z)^(2k+1) = -(z^(2k+1))
		{
			z.pull(0);
			if (!even) pack(ns().UMinus);
			change = true;
			return;
		}
		
		// |x|^2k = x^2k
		if (even && z.is_function(ns().Abs) && z.child(0).is_real())
		{
			z.pull(0);
			change = true;
		}
		
		// (-2)^2k = 2^2k
		if (even && defined(x) && x < 0.0)
		{
			z = -x;
			change = true;
		}
	}
}

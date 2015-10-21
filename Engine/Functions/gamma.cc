#include "Functions.h"

#define SQ2PI 2.5066282746310005024157652848110452530069867406099383 // √2π
#define GTR   30.0

void gamma_(const cnum &z0, cnum &ret)
{
	if (z0.real() < -5.0) // g(z) g(1-z) = π csc πz
	{
		csc(M_PI * z0, ret); ret *= M_PI;
		cnum g; gamma_(1.0-z0, g);
		ret /= g;
		return;
	}

	cnum z(z0);
	cnum df(1.0);
	if(z.real() < GTR) // g(z+n) / (n-1+z)(n-2+z)...(1+z)z = g(z)
	{
		int n = (int)(GTR - floor(z.real()));
		for (int i = 0; i < n; ++i)
		{
			df *= z + (double)i;
		}
		z += (double)n;
	}
	
	// stirling's formula
	// exp(-z) * z ^ z-0.5 = exp(lnz * (z-0.5) - z)
	ret = SQ2PI * std::exp(std::log(z) * (z-0.5) - z) / df;
	
	// series part -- 1 + 1/12z + 1 / 288z^2 - 139/51840z^3 - 571/2488320z^4 + ...
	invert(z);
	ret *= ((((-571.0 / 2488320.0)*z - (139.0 / 51840.0))*z + 1.0/288.0)*z + 1.0/12.0)*z + 1.0;
}

double gamma_(double x)
{
	if (x < -5.0) // g(z) g(1-z) = π csc πz
	{
		return M_PI * csc(M_PI * x) / gamma_(1.0 - x);
	}
	
	double df(1.0);
	if(x < GTR) // g(z+n) / (n-1+z)(n-2+z)...(1+z)z = g(z)
	{
		int n = (int)(GTR - floor(x));
		for (int i = 0; i < n; ++i)
		{
			df *= x + (double)i;
		}
		x += (double)n;
	}
	
	// stirling's formula
	// exp(-z) * z ^ z-0.5 = exp(lnz * (z-0.5) - z)
	double ret = SQ2PI * std::exp(std::log(x) * (x-0.5) - x) / df;

	// series part -- 1 + 1/12z + 1 / 288z^2 - 139/51840z^3 - 571/2488320z^4 + ...
	x = 1.0 / x;
	return ret * (((((-571.0 / 2488320.0)*x - (139.0 / 51840.0))*x + 1.0/288.0)*x + 1.0/12.0)*x + 1.0);
}

//----------------------------------------------------------------------------------------------------------------------

void   fakt(const cnum &z, cnum &r){ gamma_(z+1.0, r); }
double fakt(double x){ return gamma_(x+1.0); }

//----------------------------------------------------------------------------------------------------------------------

void digamma(const cnum &z0, cnum &ret)
{
	cnum z(z0); // digamma(z,z) is ok because of this

	if (z.real() < -5.0) // psi(z) = psi(1-z) + π cot π(1-z)
	{
		// z = 1-z
		z.real(1.0-z.real());
		z.imag(-z.imag());
		
		cot(M_PI*z, ret);
		ret *= M_PI;
	}
	else
	{
		ret = 0.0;
	}
	
	if (z.real() < GTR) // psi(z+n) - (1 / n-1+z + 1 / n-2+z + ... + 1/z) = psi(z)
	{
		unsigned n = (unsigned)(GTR - std::floor(z.real()));
		for (unsigned i = 0; i < n; ++i)
		{
			ret -= 1.0 / (z+(double)i);
		}
		z.real(z.real() + n);
	}
	
	// psi(z) ~ lnz - 1 / 2z - 1 / 12zz + 1 / 120z^4 - 1 / 252z^6 + ...
	cnum lnz; log_(z, lnz); ret += lnz;
	ret -= 0.5 / z;
	
	cnum izz = -1.0 / (z*z); // for building the 1/z^2k terms
	cnum zk = izz / 12.0;    // including coefficient
	ret += zk;
	
	zk *= izz; zk *= 0.1;
	ret += zk;
	
	zk *= izz; zk *= 120.0 / 252.0;
	ret += zk;
}

void trigamma(const cnum &z0, cnum &ret)
{
	cnum z(z0); // trigamma(z,z) is ok because of this
	bool invert;
	
	if (z.real() < -5.0) // -psi'(z) = psi'(1-z) + π cot' π(1-z) = psi'(1-z) - π^2 / sin^2 π(1-z)
	{
		// z = 1-z
		z.real(1.0-z.real());
		z.imag(-z.imag());
		
		cnum sq = std::sin(M_PI*z); sq *= sq;
		ret = (-M_PI*M_PI) / sq;
		invert = true;
	}
	else
	{
		ret = 0.0;
		invert = false;
	}
	
	if (z.real() < GTR) // psi'(z+n) + 1 / (n-1+z)^2 + 1 / (n-2+z)^2 + ... + 1/z^2 = psi'(z)
	{
		unsigned n = (unsigned)(GTR - std::floor(z.real()));
		for (unsigned i = 0; i < n; ++i)
		{
			cnum zpi(z+(double)i); zpi *= zpi;
			ret += 1.0 / zpi;
		}
		z.real(z.real() + n);
	}
	
	// psi(z) ~ 1/z + 1 / 2z^2 + 1 / 6z^3 - 1 / 30z^5 + 1 / 42z^7 - 1 / 30z^9 + ...
	cnum iz(1.0 / z);
	ret += iz;

	cnum izz = iz*iz; // for building the 1/z^2k+1 terms
	ret += 0.5 * izz;
	
	cnum zk = izz*iz / 6.0;
	ret += zk;
	
	zk *= izz; zk *= 6.0/30.0;
	ret -= zk;
	
	zk *= izz; zk *= 30.0 / 42.0;
	ret += zk;

	zk *= izz; zk *= 42.0 / 30.0;
	ret -= zk;
	
	if (invert) ret *= -1.0;
}

//----------------------------------------------------------------------------------------------------------------------

void beta(const cnum &x, const cnum &y, cnum &ret)
{
	cnum tmp;
	gamma_(x, ret);
	gamma_(x+y, tmp); ret /= tmp;
	gamma_(  y, tmp); ret *= tmp;
}

void binomco(const cnum &n, const cnum &k, cnum &ret)
{
	cnum tmp;
	gamma_(n + 1.0, ret);
	gamma_(k + 1.0, tmp); ret /= tmp;
	gamma_(n - k + 1.0, tmp); ret /= tmp;
}


#include "Functions.h"

void pdf_normal(const cnum &z, cnum &ret) // 1/sqrt(2pi) exp(- z^2 / 2)
{
	ret = std::exp(-0.5 * z * z);
	ret *= M_2_SQRTPI * M_SQRT1_2 * 0.5; // 2/sqrt(pi) * sqrt(1/2) * 1/2 = sqrt(1/2pi)
}

#if 1

#include "Faddeeva.h"
void erf_ (const cnum &z, cnum &ret){ ret = Faddeeva::erf (z, 1e-6); }
void erfc_(const cnum &z, cnum &ret){ ret = Faddeeva::erfc(z, 1e-6); }
double erf_ (double x){ return Faddeeva::erf (x); }
double erfc_(double x){ return Faddeeva::erfc(x); }

#else

void erfc_(const cnum &x, cnum &ret)
{
	double pv=9.03777677, ph=4.76888839, p0=.392213346, p1=.149181256, p2=.0215823157, p3=1.18760964e-3,
		p4=2.48565745e-5, p5=1.97879468e-7, q0=.120830487, q1=1.08747438, q2=3.02076217, q3=5.92069385,
		q4=9.78726942, q5=14.6204889, r0=.221293361, r1=.272957057, r2=6.40298026e-02, r3=5.71296931e-3,
		r4=1.93880223e-4, r5=2.50263215e-6, r6=1.22871857e-8, s1=.483321947, s2=1.93328779, s3=4.34989752,
		s4=7.73315115, s5=12.0830487, s6=17.3995901;
	
	cnum y = x * x;
	
	if (fabs(x.real()) + fabs(x.imag()) < ph)
	{
		cnum z = std::exp(pv*x);
		ret = std::exp(-y); ret *= x;
		if (z.real() >= 0.0)
		{
			ret *= (p5/(y+q5) + p4/(y+q4) + p3/(y+q3) + p2/(y+q2) + p1/(y+q1) + p0/(y+q0));
			ret += 2.0 / (1.0+z);
		}
		else
		{
			ret *= (r6/(y+s6) + r5/(y+s5) + r4/(y+s4) + r3/(y+s3) + r2/(y+s2) + r1/(y+s1) + r0/y);
			ret += 2.0 / (1.0-z);
		}
	}
	else
	{
		ret = std::exp(-y); ret *= x;
		ret *= (p5/(y+q5) + p4/(y+q4) + p3/(y+q3) + p2/(y+q2) + p1/(y+q1) + p0/(y+q0));
		if (x.real()<0) ret += 2.0;
	}
}

void erf_(const cnum &x, cnum &ret)
{
	double p0=1.12837917, p1=-.376126389, p2=.112837917, p3=-2.68661706e-02;
	if (fabs(x.real()) + fabs(x.imag()) > 0.125)
	{
		if(x.real()>=0)
		{
			erfc(x, ret);
			ret = 1.0 - ret;
		}
		else
		{
			erfc(-x, ret);
			ret -= 1.0;
		}
	}
	else
	{
		cnum y = x * x;
		ret = (((p3*y+p2)*y+p1)*y+p0)*x;
	}
}
#endif

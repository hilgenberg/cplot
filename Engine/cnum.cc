#include "cnum.h"
#include "../Utility/StringFormatting.h"
#include "Namespace/Namespace.h"
#include "Namespace/Expression.h"
#include "Namespace/Constant.h"

#define EPS 1.0e-8 // less precision gives better printing

bool prints_sign(const cnum &z)
{
	double r = z.real(), i = z.imag();
	if (isnan(r) || isinf(i) || isz(z)) return false;
	if (isinf(r)) return is_real(z);
	return fabs(r) > EPS ? r < 0.0 : i < 0.0;
}
bool prints_sum(const cnum &z)
{
	double r = z.real(), i = z.imag();
	if (isnan(r) || isinf(i) || isinf(r) || isz(z)) return false;
	return fabs(r) > EPS && fabs(i) >= EPS;
}

std::string to_string(const cnum &z, PrintingStyle ds)
{
	const char *I = (ds == PS_Mathematica ? "I" : "i");
	
	double r = z.real(), i = z.imag();
	if (isnan(r)) return "NAN";
	if (isinf(r)) return is_real(z) ? (r > 0 ? "+INF" : "-INF") : "INF";
	if (isinf(i)) return "INF";
	if (isz(z))   return "0";
	
	if (fabs(r) > EPS)
	{
		if     (fabs(i-1) <  EPS) return format("%.6g+%s", r, I);
		else if(fabs(i+1) <  EPS) return format("%.6g-%s", r, I);
		else if(     i    >  EPS) return format("%.6g+%.6g%s", r, i, I);
		else if(     i    < -EPS) return format("%.6g%.6g%s",  r, i, I);
		else                      return format("%.6g", r);
	}
	else
	{
		if     (fabs(i-1) < EPS) return  I;
		else if(fabs(i+1) < EPS) return format("-%s", I);
		else                     return format("%.6g%s", i, I);
	}
}

std::string to_string(const cnum &z, const Namespace &ns, PrintingStyle ds)
{
	const Constant *c = ns.constant(z);
	return c ? c->displayName(ds) : to_string(z, ds);
}

std::ostream & operator<< (std::ostream & out, const cnum & z)
{
	return out << to_string(z);
}

cnum evaluate(const std::string &s, const Namespace &ns)
{
	ParsingResult result;
	cnum v = Expression::parse(s, &ns, result);
	return result.ok ? v : UNDEFINED;
}

#if 0
bool cancel(double &a, double &b)
{
	return false;
}

bool cancel(cnum &a, cnum &b)
{
	bool no_r1 = isz(a.real()), no_i1 = isz(a.imag());
	bool no_r2 = isz(b.real()), no_i2 = isz(b.imag()); assert(!no_r2 || !no_i2);

	if (no_i1 && no_r1){ b = 1.0; return true; } // 0/z = 0/1
	if (no_i2 && no_r2){ a = 1.0; return true; } // z/0 = 1/0
	
	if (no_i1)
	{
		double x1 = a.real();
		
		if (no_i2)
		{
			double x2 = b.real();
			if (!cancel(x1, x2)) return false;
			a.real(x1); b.real(x2); return true;
		}
		else if (no_r2)
		{
			double y2 = b.imag();
			if (!cancel(x1, y2)) return false;
			a.real(x1); b.imag(y2); return true;
		}
		else
		{
			double x2 = b.real(), y2 = b.imag();
			if (!cancel(x1, x2, y2)) return false;
			a.real(x1); b.real(x2); b.imag(y2); return true;
		}
	}
	else if (no_r1)
	{
		double y1 = a.imag();
		
		if (no_i2)
		{
			double x2 = b.real();
			if (!cancel(y1, x2)) return false;
			a.imag(y1); b.real(x2); return true;
		}
		else if (no_r2)
		{
			double y2 = b.imag();
			if (!cancel(y1, y2)) return false;
			a.imag(y1); b.imag(y2); return true;
		}
		else
		{
			double x2 = b.real(), y2 = b.imag();
			if (!cancel(y1, x2, y2)) return false;
			a.imag(y1); b.real(x2); b.imag(y2); return true;
		}
	}
	else
	{
		double x1 = a.real(), y1 = a.imag();
		
		if (no_i2)
		{
			double x2 = b.real();
			if (!cancel(x1, y1, x2)) return false;
			a.real(x1); a.imag(y1); b.real(x2); return true;
		}
		else if (no_r2)
		{
			double y2 = b.imag();
			if (!cancel(x1, y1, y2)) return false;
			a.real(x1); a.imag(y1); b.imag(y2); return true;
		}
		else
		{
			double x2 = b.real(), y2 = b.imag();
			if (!cancel(x1, y1, x2, y2)) return false;
			a.real(x1); a.imag(y1); b.real(x2); b.imag(y2); return true;
		}
		
	}
}
#endif

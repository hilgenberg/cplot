#pragma once
#include "Element.h"
#include <vector>
class WorkingTree;

/**
 * @addtogroup Namespace
 * @{
 */
class Variable;

class Function : public Element
{
public:
	Function(const std::string &n)
	: Element(n), grd(NULL), cdiff(false), pwr(UNDEFINED), grdns(NULL)
	, inv(false), proj(false), clps(false), add(false), mul(false), lin(false)
	{ }
	~Function();
	
	virtual bool isFunction () const{ return true; }
	virtual bool base() const = 0; ///< is it an instance of BaseFunction?
	virtual int  arity() const = 0;
	virtual bool deterministic() const = 0; ///< true if the output depends only on the input (unlike random)
	virtual bool is_real(bool real_params) const{ return range(real_params ? R_Real : R_Complex) & R_Real; }
	virtual Range range(Range input) const = 0;
	
	bool involution    () const{ return inv;  } // f(f) = id
	bool projection    () const{ return proj; } // f(f) = f
	bool collapses     () const{ return clps; } // f(f) = 0
	bool additive      () const{ return add;  } // f(z1+z2) = f(z1)+f(z2)
	bool multiplicative() const{ return mul;  } // f(z1*z2) = f(z1)*f(z2)
	bool linear        () const{ return lin;  } // f(x1*z1+x2*z2) = x1*f(z1)+x2*f(z2) for real xi
	void involution    (bool f){ assert(!f || arity() == 1); inv  = f; }
	void projection    (bool f){ assert(!f || arity() == 1); proj = f; }
	void collapses     (bool f){ assert(!f || arity() == 1); clps = f; }
	void additive      (bool f){ assert(!f || arity() == 1); add  = f; }
	void multiplicative(bool f){ assert(!f || arity() == 1); mul  = f; }
	void linear        (bool f){ assert(!f || arity() == 1); lin  = f; if (f) add = f; }
	
	WorkingTree *   real_gradient(const std::vector<WorkingTree> &x) const;
	WorkingTree *complex_gradient(const std::vector<WorkingTree> &x) const;

	void gradient(const std::vector<std::string> &grad, bool complex)
	{
		assert(base() && grds.empty());
		assert((int)grad.size() == (complex ? 1 : 2) * arity());
		grds = grad; // vars must be z1=x1+iy1, z2=x2+iy2, ...
		cdiff = complex;
	}

	bool  is_power()          const{ return defined(pwr); } // is it equivalent to x^p?
	bool  is_power(double &p) const{ p = pwr; return defined(pwr); }
	void set_power(double  p){ assert(base() && arity() == 1); pwr = p; }

private:
	std::vector<std::string>        grds;
	mutable WorkingTree            *grd;
	mutable std::vector<Variable*>  grdx; // either z1,z2,... (if complex) or x1,y1,x2,y2,...
	mutable Namespace              *grdns;
	bool cdiff, inv, proj, clps, add, mul, lin;
	double pwr;
	
	bool parse_gradient() const;
};

/** @} */


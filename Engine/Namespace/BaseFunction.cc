#include "BaseFunction.h"

#include <stdexcept>

//---------------------------------------------------------------------------------------------------------------------
//  Constructors
//---------------------------------------------------------------------------------------------------------------------

BaseFunction::BaseFunction(const std::string &n, vfunc *fc,  vfuncR  *fr, bool d)
: Function(n), vfc(fc), vfr(fr), ufrc(NULL), ufcr(NULL), m_deterministic(d), m_arity(0)
{ }

BaseFunction::BaseFunction(const std::string &n, ufunc *fcc, ufuncRR *frr, ufuncCR *fcr, ufuncRC *frc, bool d)
: Function(n), ufcc(fcc), ufrr(frr), ufrc(frc), ufcr(fcr), m_deterministic(d), m_arity(1)
{ }

BaseFunction::BaseFunction(const std::string &n, bfunc *fcc, bfuncRR *frr, bfuncCR *fcr, bfuncRC *frc, bool d)
: Function(n), bfcc(fcc), bfrr(frr), bfrc(frc), bfcr(fcr), m_deterministic(d), m_arity(2)
{ }

BaseFunction::BaseFunction(const std::string &n, tfunc *fcc, tfuncRR *frr, bool d)
: Function(n), tfcc(fcc), tfrr(frr), ufrc(NULL), ufcr(NULL), m_deterministic(d), m_arity(3)
{ }

BaseFunction::BaseFunction(const std::string &n, qfunc *fcc, bool d)
: Function(n), qfcc(fcc), ufrr(NULL), ufrc(NULL), ufcr(NULL), m_deterministic(d), m_arity(4)
{ }

//---------------------------------------------------------------------------------------------------------------------
//  Operator (...) for the various arities
//---------------------------------------------------------------------------------------------------------------------

cnum BaseFunction::operator()() const
{
	CHECK_CONSISTENCY(m_arity == 0, "Function call with wrong number of arguments");
	if(vfc)
	{
		cnum ret;
		vfc(ret);	
		return ret;
	}
	if(vfr)
	{
		return vfr();
	}
	INCONSISTENCY("NULL function called");
}

cnum BaseFunction::operator()(const cnum &z1) const
{
	CHECK_CONSISTENCY(m_arity == 1, "Function call with wrong number of arguments");
	if(ufcc)
	{
		cnum ret;
		ufcc(z1, ret);
		return ret;
	}
	if(ufcr)
	{
		return cnum(ufcr(z1));
	}
	if(!::is_real(z1)) throw std::runtime_error("real function called with complex argument");
	if(ufrc)
	{
		cnum ret;
		ufrc(z1.real(), ret);
		return ret;
	}
	if(ufrr)
	{
		return cnum(ufrr(z1.real()));
	}
	INCONSISTENCY("NULL function called");
}

cnum BaseFunction::operator()(const cnum &z1, const cnum &z2) const
{
	CHECK_CONSISTENCY(m_arity == 2, "Function call with wrong number of arguments");
	if(bfcc)
	{
		cnum ret;
		bfcc(z1, z2, ret);
		return ret;
	}
	if(bfcr)
	{
		return cnum(ufcr(z1));
	}
	if(!::is_real(z1) || !::is_real(z2)) throw std::runtime_error("real function called with complex argument");
	if(bfrc)
	{
		cnum ret;
		bfrc(z1.real(), z2.real(), ret);
		return ret;
	}
	if(bfrr)
	{
		return cnum(bfrr(z1.real(), z2.real()));
	}
	INCONSISTENCY("NULL function called");
}

cnum BaseFunction::operator()(const cnum &z1, const cnum &z2, const cnum &z3) const
{
	CHECK_CONSISTENCY(m_arity == 3, "Function call with wrong number of arguments");
	if(tfcc)
	{
		cnum ret;
		tfcc(z1, z2, z3, ret);
		return ret;
	}
	INCONSISTENCY("NULL function called");
}

cnum BaseFunction::operator()(const cnum &z1, const cnum &z2, const cnum &z3, const cnum &z4) const
{
	CHECK_CONSISTENCY(m_arity == 4, "Function call with wrong number of arguments");
	if(qfcc)
	{
		cnum ret;
		qfcc(z1, z2, z3, z4, ret);
		return ret;
	}
	INCONSISTENCY("NULL function called");
}

//---------------------------------------------------------------------------------------------------------------------
//  Utility methods
//---------------------------------------------------------------------------------------------------------------------

bool BaseFunction::is_real(bool real_params) const
{
	if(m_arity == 1)
	{
		return ufcr || real_params && ufrr;
	}
	else if(m_arity == 2)
	{
		return bfcr || real_params && bfrr;
	}
	else if(m_arity == 3)
	{
		return real_params && tfrr;
	}
	return false;
}

Range BaseFunction::range(Range input) const
{
	auto i = m_range.find(input);
	if (i != m_range.end()) return i->second;
	return is_real(input & R_Real) ? R_Real : R_Complex;
}

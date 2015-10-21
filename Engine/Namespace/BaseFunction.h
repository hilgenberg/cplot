#pragma once

#include "Function.h"

#include <string>
#include <map>

/**
 * @addtogroup Namespace
 * @{
 */

typedef void   vfunc  (cnum   &);
typedef double vfuncR ();

typedef void   ufunc  (const cnum  &, cnum &);
typedef void   ufuncRC(double, cnum &);
typedef double ufuncCR(const cnum &);
typedef double ufuncRR(double);

typedef void   bfunc  (const cnum  &, const cnum  &, cnum  &);
typedef void   bfuncRC(double, double, cnum  &);
typedef double bfuncCR(const cnum  &, const cnum  &);
typedef double bfuncRR(double, double);

typedef void   tfunc  (const cnum &, const cnum &, const cnum &, cnum &);
typedef double tfuncRR(double, double, double);
typedef void qfunc(const cnum &, const cnum &, const cnum &, const cnum &, cnum &);

/**
 * BaseFunction is mostly just a function pointer in possibly several (R->R, C->C, R->C, C->R) variants.
 */

class BaseFunction : public Function
{
public:
	BaseFunction(const std::string &name, vfunc *fc,  vfuncR  *fr  = NULL, bool deterministic = false);
	BaseFunction(const std::string &name, ufunc *fcc, ufuncRR *frr = NULL, ufuncCR *fcr = NULL, ufuncRC *frc = NULL, bool deterministic = true);
	BaseFunction(const std::string &name, bfunc *fcc, bfuncRR *frr = NULL, bfuncCR *fcr = NULL, bfuncRC *frc = NULL, bool deterministic = true);
	BaseFunction(const std::string &name, tfunc *fcc, tfuncRR *frr = NULL, bool deterministic = true);
	BaseFunction(const std::string &name, qfunc *fcc, bool deterministic = true);

	virtual bool base() const{ return true; }
	virtual int  arity() const{ return m_arity; }
	virtual bool deterministic() const{ return m_deterministic; }
	virtual bool builtin() const{ return true; }
	virtual bool is_real(bool real_params) const;
	virtual Range range(Range input) const;
	void range(Range input, Range output){ m_range.insert(std::make_pair(input, output)); }

	cnum operator()() const;
	cnum operator()(const cnum &z1) const;
	cnum operator()(const cnum &z1, const cnum &z2) const;
	cnum operator()(const cnum &z1, const cnum &z2, const cnum &z3) const;
	cnum operator()(const cnum &z1, const cnum &z2, const cnum &z3, const cnum &z4) const;
	
	union
	{
		vfunc *const vfc;
		ufunc *const ufcc;
		bfunc *const bfcc;
		tfunc *const tfcc;
		qfunc *const qfcc;
	};
	union
	{
		vfuncR  *const vfr;
		ufuncRR *const ufrr;
		bfuncRR *const bfrr;
		tfuncRR *const tfrr;
	};
	union
	{
		ufuncRC *const ufrc;
		bfuncRC *const bfrc;
	};
	union
	{
		ufuncCR *const ufcr;
		bfuncCR *const bfcr;
	};

#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "BaseFunction " << name(); }
#endif

protected:
	virtual Element *copy() const{ return NULL; }

private:
	const bool m_deterministic;
	const int  m_arity;
	std::map<Range, Range> m_range;
};

/** @} */


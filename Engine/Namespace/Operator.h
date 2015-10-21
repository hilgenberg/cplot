#pragma once

#include "BaseFunction.h"

/**
 * @addtogroup Namespace
 * @{
 */


class Operator : public BaseFunction
{
public:
	Operator(const std::string &name, bfunc *fcc, bfuncRR *frr = NULL, bfuncCR *fcr = NULL, bfuncRC *frc = NULL,
			 bool deterministic = true)
	: BaseFunction(name, fcc, frr, fcr, frc, deterministic) { }
	
	Operator(const std::string &name, ufunc *fcc, ufuncRR *frr = NULL, ufuncCR *fcr = NULL, ufuncRC *frc = NULL,
			 bool deterministic = true)
	: BaseFunction(name, fcc, frr, fcr, frc, deterministic) { }

	virtual bool isFunction () const{ return false; }
	virtual bool isOperator () const{ return true;  }
	
	virtual bool   unary() const = 0;
	bool          binary() const{ return !unary(); }
	virtual bool  prefix() const{ return false; }
	virtual bool postfix() const{ return false; }
	virtual int  arity()   const{ return unary() ? 1 : 2; }
};

class UnaryOperator : public Operator
{
public:
	UnaryOperator(const std::string &n, bool prefix,
				  ufunc *fcc, ufuncRR *frr = NULL, ufuncCR *fcr = NULL, ufuncRC *frc = NULL,
				  bool deterministic = true)
	: Operator(n, fcc, frr, fcr, frc, deterministic), pre(prefix) { }
	
	virtual bool   unary() const{ return true; }
	virtual bool  prefix() const{ return  pre; }
	virtual bool postfix() const{ return !pre; }

private:
	const bool pre;
};

class BinaryOperator : public Operator
{
public:
	BinaryOperator(const std::string &name, int precedence, bool logical,
				   bool associative, bool commutative, bool rightbinding,
				   bfunc *fcc, bfuncRR *frr = NULL, bfuncCR *fcr = NULL, bfuncRC *frc = NULL,
				   bool deterministic = true)
	: Operator(name, fcc, frr, fcr, frc, deterministic),
	  m_precedence(precedence), m_logical(logical), m_associative(associative),
	  m_commutative(commutative), m_rightbinding(rightbinding) { }

	virtual bool unary() const{ return false;          }
	bool       logical() const{ return m_logical;      } ///< Result is always 0 or 1
	bool   associative() const{ return m_associative;  } ///< A∘(B∘C) = (A∘B)∘C
	bool   commutative() const{ return m_commutative;  } ///< A∘B = B∘A
	bool  rightbinding() const{ return m_rightbinding; } ///< True if (A∘B∘C) should be (A∘(B∘C))
	int     precedence() const{ return m_precedence;   }
	
private:
	const bool m_logical, m_associative, m_commutative, m_rightbinding;
	const int  m_precedence;
};

/** @} */


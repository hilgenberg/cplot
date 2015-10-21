#pragma once

#include "Namespace.h"
#include "../Parser/Simplifier/Pattern.h"

#include <string>
#include <vector>
#include <map>

class UnaryOperator;
class BinaryOperator;
class BaseFunction;
class Constant;

/**
 * @addtogroup Namespace
 * @{
 */


class RootNamespace : public Namespace
{
public:
	RootNamespace(); // adds all builtin functions, operators and constants
	virtual TypeCode type_code() const{ return TypeCode::INVALID; }
	virtual bool        isRoot() const{ return true; }

	virtual bool builtin() const{ return true; }

	virtual void  clear(); // leaves the standard functions

	virtual void candidates(const std::string &s, size_t pos, std::vector<Element*> &ret) const;
	virtual void rcandidates(const std::string &s, size_t pos, size_t n, std::vector<Element*> &ret) const;

	virtual RootNamespace *root_container() const{ return const_cast<RootNamespace*>(this); }

	BinaryOperator *Pow, *PowStar, *Plus, *Minus, *Mul, *Div;
	BaseFunction   *Identity;
	BaseFunction   *Combine; // x+iy
	BaseFunction   *Abs, *Sqrt, *Re, *Im, *Conj, *Hypot, *Exp, *Log, *Log_;
	UnaryOperator  *UPlus, *UMinus, *Invert; // unary + - 1/
	UnaryOperator  *ConjOp;
	UnaryOperator  *PowOps[10];
	Constant       *I;
	
	static inline int ImplicitMultiplicationPrecedence(){ return 10; }
	
	enum Combination
	{
		Nothing = 0, // f(g) can not be simplified
		Commutes,    // f(g) = g(f)
		Collapses,   // f(g) = id
		First,       // f(g) = f
		Second,      // f(g) = g
		Zero,        // f(g) = 0
		One          // f(g) = 1
	};
	Combination combine(const Element *f, const Element *g) const; // f(g) = ?
	
	const std::vector<Rule> &rules(const Element *head) const;

private:
	std::map<std::pair<const Element*, const Element*>, Combination> combinations;

	void combines(const Element *f, const Element *g, Combination c, bool symmetric = false)
	{
		assert(f->arity() == 1);
		assert(c != Collapses || c != First || g->arity() == 1);
		combinations.emplace(std::make_pair(std::make_pair(f,g), c));
		if (symmetric)
		{
			assert(g->arity() == 1);
			combinations.emplace(std::make_pair(std::make_pair(g,f), c));
		}
	}
	inline void commutes(const Element *f, const Element *g){ combines(f, g, Commutes, true); }

	inline void even(const Element *f){ combines(f, (Element*)UMinus, First); }
	inline void odd (const Element *f){ commutes(f, (Element*)UMinus); }
	inline void inverse(const Element *f, const Element *g, bool symmetric = false) // f(g) = id
	{
		combines(f, g, Collapses, symmetric);
	}

	mutable std::map<const Element *, std::vector<Rule>> m_rules;
};

/** @} */

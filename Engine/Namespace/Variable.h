#pragma once

#include "Namespace.h"

/**
 * @addtogroup Namespace
 * @{
 */


/**
 * Symbol with no fixed value. Can be real or complex.
 */

class Variable : public Element
{
public:
	Variable(const std::string &name, bool is_real) : Element(name), is_real(is_real){ }
	
	virtual bool isVariable () const{ return true; }
	virtual bool builtin() const{ return false; }
	virtual int  arity() const{ return -1; }

	bool real() const{ return is_real; }
	inline Range range() const
	{
		return is_real ? R_Real : R_Complex;
	}

#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "Variable " << name(); }
#endif

protected:
	bool is_real;
	
	virtual Element *copy() const
	{
		return new Variable(name(), is_real);
	}
};

/** @} */


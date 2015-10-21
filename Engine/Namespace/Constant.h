#pragma once

#include "Namespace.h"

/**
 * @addtogroup Namespace
 * @{
 */


/**
 * Symbol with a fixed value. Always live in the root namespace as builtins.
 * These are not user-creatable (unlike Parameters).
 */

class Constant : public Element
{
public:
	Constant(const std::string &n, const cnum &v) : Element(n), v(v) { };

	virtual bool isConstant() const{ return true; }
	virtual bool builtin() const{ return true; }
	virtual int  arity() const{ return -1; }
	
	bool        real () const{ return is_real(v); }
	const cnum &value() const{ return v; }

	inline Range range() const{ return ::range(v); }

#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "Constant " << name() << " = " << v; }
#endif

protected:
	const cnum v;
	
	virtual Element *copy() const{ return NULL; }
};

/** @} */


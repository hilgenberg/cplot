#pragma once

#include "Namespace.h"
#include "../Parser/WorkingTree.h"

#include <cassert>

class Variable;
class Expression;

/**
 * @addtogroup Namespace
 * @{
 */


/**
 * Dependent variable. All occurrences of it will be replaced by some replacement.
 */

class AliasVariable : public Element
{
public:
	AliasVariable(const std::string &n, Variable *re); ///< Replacement is another variable
	AliasVariable(const std::string &n, Variable *re, Variable *im); ///< Replace with re + i*im
	AliasVariable(const std::string &n, const Expression &ex); ///< Replace with expression
	AliasVariable(const std::string &n, AliasVariable *x); ///< Replacement is another alias
	
	virtual bool isAlias() const{ return true; }
	virtual bool builtin() const{ return true; }
	virtual int  arity()   const{ return -1; }

	const WorkingTree &replacement() const{ return m_replacement; }
	
#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "AliasVariable " << name(); }
#endif
	
protected:
	virtual Element *copy() const{ return NULL; }

private:
	WorkingTree m_replacement;
};

/** @} */


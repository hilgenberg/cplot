#pragma once

#include "Function.h"
#include "Namespace.h"

#include <vector>
#include <set>

class Expression;
class Variable;

/**
 * @addtogroup Namespace
 * @{
 */


/**
 * Function defined by an expression.
 *
 * Syntax for function strings: name(v1, v2, ..., vn) [:]= Expression <BR>
 * The v_i must be distinct and name must be Namespace::valid_name
 */

class UserFunction : public Function, public IDCarrier
{
public:
	UserFunction();
	~UserFunction();
	UserFunction &operator=(const UserFunction &f);
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	virtual TypeCode type_code() const{ return TypeCode::FUNCTION; }
	virtual bool builtin() const{ return false; } // for now this is always false
	
	virtual bool  base() const{ return false; }
	virtual int  arity() const{ return m_arity; }
	virtual bool deterministic() const{ if (dirty) parse(); return m_deterministic; }
	virtual bool is_real(bool real_params) const;
	virtual Range range(Range input) const;

	bool valid() const;
	
	std::string formula() const{ return m_formula; }
	void formula(const std::string &f); ///< Also updates name, arity, ...
	
	Expression *expression() const{ if (dirty) parse(); return fx; }
	const std::vector<Variable*> &arguments() const{ if (dirty) parse(); return args; }

	WorkingTree *apply(const std::vector<WorkingTree*> &args) const;
	WorkingTree *apply(const std::vector<WorkingTree>  &args) const;

	virtual void redefinition(const std::set<std::string> &affected_names);
#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "UserFunction " << name(); }
#endif

protected:
	virtual Element *copy() const;
	virtual void added_to_namespace();

private:
	std::string m_formula;               ///< The entire thing, as entered by the user.
	mutable bool dirty;                  ///< Expression needs parsing
	mutable size_t nameIndex, nameLen;   ///< Position of name in formula
	mutable size_t exprIndex;            ///< Start of expression
	mutable std::vector<Variable*> args; ///< Input variables (v1, ..., vn)
	mutable Expression *fx;              ///< Definition, element of ins
	mutable Namespace   ins;             ///< Internal namespace, linked to container(), owns the vi and fx

	mutable int m_arity;
	mutable int m_deterministic;
	
	void parse() const; ///< Create/update fx
	bool recursive() const; ///< True if it would call itself, which would be infinite recursion.
	void collect(std::set<const UserFunction*> &functions) const; ///< Find all userfunctions this will call.
};

/** @} */

